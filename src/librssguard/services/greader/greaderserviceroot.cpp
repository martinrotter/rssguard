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

GreaderServiceRoot::~GreaderServiceRoot() {}

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
  return false;
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

  /*
     QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

     // Save the actual data read/unread.
     while (i.hasNext()) {
     i.next();
     auto key = i.key();
     QStringList ids = i.value();

     if (!ids.isEmpty()) {
      auto res = network()->markMessagesRead(key, ids, networkProxy());

      if (!ignore_errors && res.first != QNetworkReply::NetworkError::NoError) {
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
      QStringList feed_ids, guid_hashes;

      for (const Message& msg : messages) {
        feed_ids.append(msg.m_feedId);
        guid_hashes.append(msg.m_customHash);
      }

      auto res = network()->markMessagesStarred(key, feed_ids, guid_hashes, networkProxy());

      if (!ignore_errors && res.first != QNetworkReply::NetworkError::NoError) {
        addMessageStatesToCache(messages, key);
      }
     }
     }*/
}

void GreaderServiceRoot::updateTitle() {
  setTitle(QString("%1 (%2)").arg(m_network->username(),
                                  m_network->serviceToString(m_network->service())));
}

void GreaderServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    if (DatabaseQueries::overwriteGreaderAccount(database, m_network->username(),
                                                 m_network->password(), m_network->baseUrl(),
                                                 m_network->batchSize(), accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    if (DatabaseQueries::createGreaderAccount(database, accountId(), m_network->username(),
                                              m_network->password(), m_network->service(),
                                              m_network->baseUrl(), m_network->batchSize())) {
      updateTitle();
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
