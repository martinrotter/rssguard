// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlyserviceroot.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlyentrypoint.h"
#include "services/feedly/feedlyfeed.h"
#include "services/feedly/feedlynetwork.h"
#include "services/feedly/gui/formeditfeedlyaccount.h"

#if defined (FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

FeedlyServiceRoot::FeedlyServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new FeedlyNetwork(this)) {
  setIcon(FeedlyEntryPoint().icon());
  m_network->setService(this);
}

bool FeedlyServiceRoot::isSyncable() const {
  return true;
}

bool FeedlyServiceRoot::canBeEdited() const {
  return true;
}

bool FeedlyServiceRoot::canBeDeleted() const {
  return true;
}

bool FeedlyServiceRoot::editViaGui() {
  FormEditFeedlyAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

bool FeedlyServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteFeedlyAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

void FeedlyServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile();

  if (childCount() <= 3) {
    syncIn();
  }
}

QString FeedlyServiceRoot::code() const {
  return FeedlyEntryPoint().code();
}

void FeedlyServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      try {
        network()->markers(key == RootItem::ReadStatus::Read
                           ? FEEDLY_MARKERS_READ
                           : FEEDLY_MARKERS_UNREAD, ids);
      }
      catch (const NetworkException& net_ex) {
        qCriticalNN << LOGSEC_FEEDLY
                    << "Failed to synchronize read/unread state with error:"
                    << QUOTE_W_SPACE(net_ex.message())
                    << "and HTTP code"
                    << QUOTE_W_SPACE_DOT(net_ex.networkError());

        if (!ignore_errors) {
          addMessageStatesToCache(ids, key);
        }
      }
    }
  }

  QMapIterator<RootItem::Importance, QList<Message>> j(msg_cache.m_cachedStatesImportant);

  // Save the actual data important/not important.
  while (j.hasNext()) {
    j.next();
    auto key = j.key();
    QList<Message> messages = j.value();

    if (!messages.isEmpty()) {
      QStringList ids;

      for (const Message& msg : messages) {
        ids.append(msg.m_customId);
      }

      try {
        network()->markers(key == RootItem::Importance::Important
                           ? FEEDLY_MARKERS_IMPORTANT
                           : FEEDLY_MARKERS_UNIMPORTANT, ids);
      }
      catch (const NetworkException& net_ex) {
        qCriticalNN << LOGSEC_FEEDLY
                    << "Failed to synchronize important/unimportant state with error:"
                    << QUOTE_W_SPACE(net_ex.message())
                    << "and HTTP code"
                    << QUOTE_W_SPACE_DOT(net_ex.networkError());

        if (!ignore_errors) {
          addMessageStatesToCache(messages, key);
        }
      }
    }
  }

  QMapIterator<QString, QStringList> k(msg_cache.m_cachedLabelAssignments);

  // Assign label for these messages.
  while (k.hasNext()) {
    k.next();
    auto label_custom_id = k.key();
    QStringList messages = k.value();

    if (!messages.isEmpty()) {
      try {
        network()->tagEntries(label_custom_id, messages);
      }
      catch (const NetworkException& net_ex) {
        qCriticalNN << LOGSEC_FEEDLY
                    << "Failed to synchronize tag assignments with error:"
                    << QUOTE_W_SPACE(net_ex.message())
                    << "and HTTP code"
                    << QUOTE_W_SPACE_DOT(net_ex.networkError());

        if (!ignore_errors) {
          addLabelsAssignmentsToCache(messages, label_custom_id, true);
        }
      }
    }
  }

  QMapIterator<QString, QStringList> l(msg_cache.m_cachedLabelDeassignments);

  // Remove label from these messages.
  while (l.hasNext()) {
    l.next();
    auto label_custom_id = l.key();
    QStringList messages = l.value();

    if (!messages.isEmpty()) {
      try {
        network()->untagEntries(label_custom_id, messages);
      }
      catch (const NetworkException& net_ex) {
        qCriticalNN << LOGSEC_FEEDLY
                    << "Failed to synchronize tag DEassignments with error:"
                    << QUOTE_W_SPACE(net_ex.message())
                    << "and HTTP code"
                    << QUOTE_W_SPACE_DOT(net_ex.networkError());

        if (!ignore_errors) {
          addLabelsAssignmentsToCache(messages, label_custom_id, false);
        }
      }
    }
  }
}

ServiceRoot::LabelOperation FeedlyServiceRoot::supportedLabelOperations() const {
  return LabelOperation(0);
}

void FeedlyServiceRoot::updateTitle() {
  setTitle(QString("%1 (Feedly)").arg(TextFactory::extractUsernameFromEmail(m_network->username())));
}

void FeedlyServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    if (DatabaseQueries::overwriteFeedlyAccount(database,
                                                m_network->username(),
                                                m_network->developerAccessToken(),
#if defined (FEEDLY_OFFICIAL_SUPPORT)
                                                m_network->oauth()->refreshToken(),
#else
                                                {},
#endif
                                                m_network->batchSize(),
                                                m_network->downloadOnlyUnreadMessages(),
                                                accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    if (DatabaseQueries::createFeedlyAccount(database,
                                             m_network->username(),
                                             m_network->developerAccessToken(),
#if defined (FEEDLY_OFFICIAL_SUPPORT)
                                             m_network->oauth()->refreshToken(),
#else
                                             {},
#endif
                                             m_network->batchSize(),
                                             m_network->downloadOnlyUnreadMessages(),
                                             accountId())) {
      updateTitle();
    }
  }
}

RootItem* FeedlyServiceRoot::obtainNewTreeForSyncIn() const {
  try {
    auto tree = m_network->collections(true);
    auto* lblroot = new LabelsNode(tree);
    auto labels = m_network->tags();

    lblroot->setChildItems(labels);
    tree->appendChild(lblroot);

    return tree;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_FEEDLY
                << "Failed to obtain new sync-in tree:"
                << QUOTE_W_SPACE_DOT(ex.message());
    return nullptr;
  }
}

void FeedlyServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<FeedlyFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);
}
