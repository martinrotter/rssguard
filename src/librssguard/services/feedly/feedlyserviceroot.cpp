// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlyserviceroot.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedfetchexception.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlyentrypoint.h"
#include "services/feedly/feedlynetwork.h"
#include "services/feedly/gui/formeditfeedlyaccount.h"

#if defined(FEEDLY_OFFICIAL_SUPPORT)
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

bool FeedlyServiceRoot::editViaGui() {
  FormEditFeedlyAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

QVariantHash FeedlyServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data["username"] = m_network->username();
  data["dat"] = m_network->developerAccessToken();

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  data["refresh_token"] = m_network->oauth()->refreshToken();
#endif

  data["batch_size"] = m_network->batchSize();
  data["download_only_unread"] = m_network->downloadOnlyUnreadMessages();

  return data;
}

void FeedlyServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setUsername(data["username"].toString());
  m_network->setDeveloperAccessToken(data["dat"].toString());

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  m_network->oauth()->setRefreshToken(data["refresh_token"].toString());
#endif

  m_network->setBatchSize(data["batch_size"].toInt());
  m_network->setDownloadOnlyUnreadMessages(data["download_only_unread"].toBool());
}

QList<Message> FeedlyServiceRoot::obtainNewMessages(Feed* feed,
                                                    const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                                    const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)

  try {
    return m_network->streamContents(feed->customId());
  }
  catch (const ApplicationException& ex) {
    throw FeedFetchException(Feed::Status::NetworkError, ex.message());
  }
}

void FeedlyServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadFromDatabase<Category, Feed>(this);
    loadCacheFromFile();
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
#if defined(FEEDLY_OFFICIAL_SUPPORT)
    m_network->oauth()->login([this]() {
      syncIn();
    });
#else
    syncIn();
#endif
  }

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  else {
    m_network->oauth()->login();
  }
#endif
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
  return ServiceRoot::LabelOperation::Synchronised;
}

void FeedlyServiceRoot::updateTitle() {
  setTitle(QString("%1 (Feedly)").arg(TextFactory::extractUsernameFromEmail(m_network->username())));
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
