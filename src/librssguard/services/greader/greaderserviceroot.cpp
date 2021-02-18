// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greaderserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/greader/greaderentrypoint.h"
#include "services/greader/greaderfeed.h"
#include "services/greader/greadernetwork.h"
#include "services/greader/gui/formeditgreaderaccount.h"

GreaderServiceRoot::GreaderServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new GreaderNetwork(this)) {
  setIcon(GreaderEntryPoint().icon());
}

bool GreaderServiceRoot::isSyncable() const {
  return true;
}

bool GreaderServiceRoot::canBeEdited() const {
  return true;
}

bool GreaderServiceRoot::canBeDeleted() const {
  return true;
}

bool GreaderServiceRoot::editViaGui() {
  FormEditGreaderAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

bool GreaderServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteGreaderAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

void GreaderServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile();

  if (childCount() <= 3) {
    syncIn();
  }
}

QString GreaderServiceRoot::code() const {
  return GreaderEntryPoint().code();
}

void GreaderServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      if (network()->markMessagesRead(key, ids, networkProxy()) != QNetworkReply::NetworkError::NoError &&
          !ignore_errors) {
        addMessageStatesToCache(ids, key);
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
      QStringList custom_ids;

      for (const Message& msg : messages) {
        custom_ids.append(msg.m_customId);
      }

      if (network()->markMessagesStarred(key, custom_ids, networkProxy()) != QNetworkReply::NetworkError::NoError &&
          !ignore_errors) {
        addMessageStatesToCache(messages, key);
      }
    }
  }

  if (m_network->service() != Service::TheOldReader) {
    // The Old Reader does not support labels.
    QMapIterator<QString, QStringList> k(msg_cache.m_cachedLabelAssignments);

    // Assign label for these messages.
    while (k.hasNext()) {
      k.next();
      auto label_custom_id = k.key();
      QStringList messages = k.value();

      if (!messages.isEmpty()) {
        if (network()->editLabels(label_custom_id, true, messages, networkProxy()) != QNetworkReply::NetworkError::NoError &&
            !ignore_errors) {
          addLabelsAssignmentsToCache(messages, label_custom_id, true);
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
        if (network()->editLabels(label_custom_id, false, messages, networkProxy()) != QNetworkReply::NetworkError::NoError &&
            !ignore_errors) {
          addLabelsAssignmentsToCache(messages, label_custom_id, false);
        }
      }
    }
  }
}

ServiceRoot::LabelOperation GreaderServiceRoot::supportedLabelOperations() const {
  return LabelOperation(0);
}

void GreaderServiceRoot::updateTitleIcon() {
  setTitle(QString("%1 (%2)").arg(TextFactory::extractUsernameFromEmail(m_network->username()),
                                  m_network->serviceToString(m_network->service())));

  switch (m_network->service()) {
    case Service::TheOldReader:
      setIcon(qApp->icons()->miscIcon(QSL("theoldreader")));
      break;

    case Service::FreshRss:
      setIcon(qApp->icons()->miscIcon(QSL("freshrss")));
      break;

    case Service::Bazqux:
      setIcon(qApp->icons()->miscIcon(QSL("bazqux")));
      break;

    default:
      setIcon(GreaderEntryPoint().icon());
      break;
  }
}

void GreaderServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    if (DatabaseQueries::overwriteGreaderAccount(database, m_network->username(),
                                                 m_network->password(), m_network->service(),
                                                 m_network->baseUrl(), m_network->batchSize(),
                                                 accountId())) {
      updateTitleIcon();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    if (DatabaseQueries::createGreaderAccount(database, accountId(), m_network->username(),
                                              m_network->password(), m_network->service(),
                                              m_network->baseUrl(), m_network->batchSize())) {
      updateTitleIcon();
    }
  }
}

RootItem* GreaderServiceRoot::obtainNewTreeForSyncIn() const {
  return m_network->categoriesFeedsLabelsTree(true, networkProxy());
}

void GreaderServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<GreaderFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);
}
