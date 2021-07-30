// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/owncloudserviceroot.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/feedfetchexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"
#include "services/owncloud/owncloudfeed.h"
#include "services/owncloud/owncloudnetworkfactory.h"
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

bool OwnCloudServiceRoot::editViaGui() {
  QScopedPointer<FormEditOwnCloudAccount> form_pointer(new FormEditOwnCloudAccount(qApp->mainFormWidget()));

  form_pointer->addEditAccount(this);
  return true;
}

bool OwnCloudServiceRoot::supportsFeedAdding() const {
  return false;
}

bool OwnCloudServiceRoot::supportsCategoryAdding() const {
  return false;
}

void OwnCloudServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadFromDatabase<Category, OwnCloudFeed>(this);
    loadCacheFromFile();
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
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

RootItem* OwnCloudServiceRoot::obtainNewTreeForSyncIn() const {
  OwnCloudGetFeedsCategoriesResponse feed_cats_response = m_network->feedsCategories(networkProxy());

  if (feed_cats_response.networkError() == QNetworkReply::NetworkError::NoError) {
    return feed_cats_response.feedsCategories(true);
  }
  else {
    return nullptr;
  }
}

QVariantHash OwnCloudServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data["auth_username"] = m_network->authUsername();
  data["auth_password"] = TextFactory::encrypt(m_network->authPassword());
  data["url"] = m_network->url();
  data["force_update"] = m_network->forceServerSideUpdate();
  data["batch_size"] = m_network->batchSize();
  data["download_only_unread"] = m_network->downloadOnlyUnreadMessages();

  return data;
}

void OwnCloudServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setAuthUsername(data["auth_username"].toString());
  m_network->setAuthPassword(TextFactory::decrypt(data["auth_password"].toString()));
  m_network->setUrl(data["url"].toString());
  m_network->setForceServerSideUpdate(data["force_update"].toBool());
  m_network->setBatchSize(data["batch_size"].toInt());
  m_network->setDownloadOnlyUnreadMessages(data["download_only_unread"].toBool());
}

QList<Message> OwnCloudServiceRoot::obtainNewMessages(Feed* feed,
                                                      const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                                      const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)

  OwnCloudGetMessagesResponse messages = network()->getMessages(feed->customNumericId(), networkProxy());

  if (messages.networkError() != QNetworkReply::NetworkError::NoError) {
    throw FeedFetchException(Feed::Status::NetworkError);
  }
  else {
    return messages.messages();
  }
}
