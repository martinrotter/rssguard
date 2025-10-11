// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/nextcloudserviceroot.h"

#include "src/gui/formeditnextcloudaccount.h"
#include "src/nextcloudfeed.h"
#include "src/nextcloudnetworkfactory.h"
#include "src/nextcloudserviceentrypoint.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/feedfetchexception.h>
#include <librssguard/exceptions/networkexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/textfactory.h>

NextcloudServiceRoot::NextcloudServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new NextcloudNetworkFactory()) {
  setIcon(NextcloudServiceEntryPoint().icon());
}

NextcloudServiceRoot::~NextcloudServiceRoot() {
  delete m_network;
}

bool NextcloudServiceRoot::isSyncable() const {
  return true;
}

bool NextcloudServiceRoot::canBeEdited() const {
  return true;
}

FormAccountDetails* NextcloudServiceRoot::accountSetupDialog() const {
  return new FormEditNextcloudAccount(qApp->mainFormWidget());
}

void NextcloudServiceRoot::editItems(const QList<RootItem*>& items) {
  if (items.first()->kind() == RootItem::Kind::ServiceRoot) {
    QScopedPointer<FormEditNextcloudAccount> p(qobject_cast<FormEditNextcloudAccount*>(accountSetupDialog()));

    p->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

bool NextcloudServiceRoot::supportsFeedAdding() const {
  return false;
}

bool NextcloudServiceRoot::supportsCategoryAdding() const {
  return false;
}

void NextcloudServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, NextcloudFeed>(this);
    loadCacheFromFile();
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
    syncIn();
  }
}

QString NextcloudServiceRoot::code() const {
  return NextcloudServiceEntryPoint().code();
}

NextcloudNetworkFactory* NextcloudServiceRoot::network() const {
  return m_network;
}

void NextcloudServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      auto res = network()->markMessagesRead(key, ids, networkProxy());

      if (!ignore_errors && res.m_networkError != QNetworkReply::NetworkError::NoError) {
        addMessageStatesToCache(ids, key);
      }
    }
  }

  QMapIterator<RootItem::Importance, QList<Message>> j(msg_cache.m_cachedStatesImportant);
  QHash<int, Feed*> hashed_feeds = getPrimaryIdHashedSubTreeFeeds();

  // Save the actual data important/not important.
  while (j.hasNext()) {
    j.next();
    auto key = j.key();
    QList<Message> messages = j.value();

    if (!messages.isEmpty()) {
      QStringList feed_ids, guid_hashes;

      for (const Message& msg : messages) {
        // TODO: check this.
        feed_ids.append(hashed_feeds.value(msg.m_feedId)->customId());
        guid_hashes.append(msg.m_customHash);
      }

      auto res = network()->markMessagesStarred(key, feed_ids, guid_hashes, networkProxy());

      if (!ignore_errors && res.m_networkError != QNetworkReply::NetworkError::NoError) {
        addMessageStatesToCache(messages, key);
      }
    }
  }
}

void NextcloudServiceRoot::updateTitle() {
  setTitle(m_network->authUsername() + QSL(" (Nextcloud News)"));
}

RootItem* NextcloudServiceRoot::obtainNewTreeForSyncIn() const {
  NextcloudGetFeedsCategoriesResponse feed_cats_response = m_network->feedsCategories(networkProxy());

  if (feed_cats_response.networkError() == QNetworkReply::NetworkError::NoError) {
    return feed_cats_response.feedsCategories(true);
  }
  else {
    throw NetworkException(feed_cats_response.networkError(),
                           tr("cannot get list of feeds, network error '%1'").arg(feed_cats_response.networkError()));
  }
}

QVariantHash NextcloudServiceRoot::customDatabaseData() const {
  QVariantHash data = ServiceRoot::customDatabaseData();

  data[QSL("auth_username")] = m_network->authUsername();
  data[QSL("auth_password")] = TextFactory::encrypt(m_network->authPassword());
  data[QSL("url")] = m_network->url();
  data[QSL("force_update")] = m_network->forceServerSideUpdate();
  data[QSL("batch_size")] = m_network->batchSize();
  data[QSL("download_only_unread")] = m_network->downloadOnlyUnreadMessages();

  return data;
}

void NextcloudServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  ServiceRoot::setCustomDatabaseData(data);

  m_network->setAuthUsername(data[QSL("auth_username")].toString());
  m_network->setAuthPassword(TextFactory::decrypt(data[QSL("auth_password")].toString()));
  m_network->setUrl(data[QSL("url")].toString());
  m_network->setForceServerSideUpdate(data[QSL("force_update")].toBool());
  m_network->setBatchSize(data[QSL("batch_size")].toInt());
  m_network->setDownloadOnlyUnreadMessages(data[QSL("download_only_unread")].toBool());
}

QList<Message> NextcloudServiceRoot::obtainNewMessages(Feed* feed,
                                                       const QHash<ServiceRoot::BagOfMessages, QStringList>&
                                                         stated_messages,
                                                       const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)

  NextcloudGetMessagesResponse messages = network()->getMessages(feed->customNumericId(), networkProxy());

  if (messages.networkError() != QNetworkReply::NetworkError::NoError) {
    throw FeedFetchException(Feed::Status::NetworkError);
  }
  else {
    return messages.messages();
  }
}
