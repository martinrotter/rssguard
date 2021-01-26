// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/owncloudserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/owncloudfeed.h"
#include "services/owncloud/owncloudserviceentrypoint.h"

OwnCloudServiceRoot::OwnCloudServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new OwnCloudNetworkFactory()) {
  setIcon(OwnCloudServiceEntryPoint().icon());
}

OwnCloudServiceRoot::~OwnCloudServiceRoot() {
  delete m_network;
}

bool OwnCloudServiceRoot::isSyncable() const {
  return true;
}

bool OwnCloudServiceRoot::canBeEdited() const {
  return true;
}

bool OwnCloudServiceRoot::canBeDeleted() const {
  return true;
}

bool OwnCloudServiceRoot::editViaGui() {
  QScopedPointer<FormEditOwnCloudAccount> form_pointer(new FormEditOwnCloudAccount(qApp->mainFormWidget()));

  form_pointer->addEditAccount(this);
  return true;
}

bool OwnCloudServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteOwnCloudAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

bool OwnCloudServiceRoot::supportsFeedAdding() const {
  return false;
}

bool OwnCloudServiceRoot::supportsCategoryAdding() const {
  return false;
}

void OwnCloudServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile();

  if (childCount() <= 3) {
    syncIn();
  }
}

QString OwnCloudServiceRoot::code() const {
  return OwnCloudServiceEntryPoint().code();
}

OwnCloudNetworkFactory* OwnCloudServiceRoot::network() const {
  return m_network;
}

void OwnCloudServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
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
  }
}

void OwnCloudServiceRoot::updateTitle() {
  setTitle(m_network->authUsername() + QSL(" (Nextcloud News)"));
}

void OwnCloudServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    if (DatabaseQueries::overwriteOwnCloudAccount(database, m_network->authUsername(),
                                                  m_network->authPassword(), m_network->url(),
                                                  m_network->forceServerSideUpdate(), m_network->batchSize(),
                                                  m_network->downloadOnlyUnreadMessages(), accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    if (DatabaseQueries::createOwnCloudAccount(database, accountId(), m_network->authUsername(),
                                               m_network->authPassword(), m_network->url(),
                                               m_network->forceServerSideUpdate(),
                                               m_network->downloadOnlyUnreadMessages(),
                                               m_network->batchSize())) {
      updateTitle();
    }
  }
}

RootItem* OwnCloudServiceRoot::obtainNewTreeForSyncIn() const {
  OwnCloudGetFeedsCategoriesResponse feed_cats_response = m_network->feedsCategories(networkProxy());

  if (feed_cats_response.networkError() == QNetworkReply::NetworkError::NoError) {
    return feed_cats_response.feedsCategories(true);
  }
  else {
    return nullptr;
  }
}

void OwnCloudServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<OwnCloudFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);
}
