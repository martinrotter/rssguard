// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/nextcloudserviceroot.h"

#include "src/gui/formeditnextcloudaccount.h"
#include "src/gui/formnextcloudfeeddetails.h"
#include "src/nextcloudfeed.h"
#include "src/nextcloudnetworkfactory.h"
#include "src/nextcloudserviceentrypoint.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/feedfetchexception.h>
#include <librssguard/exceptions/networkexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/mutex.h>
#include <librssguard/miscellaneous/textfactory.h>

NextcloudServiceRoot::NextcloudServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new NextcloudNetworkFactory()) {
  setIcon(NextcloudServiceEntryPoint().icon());

  // connect(this, &NextcloudServiceRoot::syncInRequested, this, &NextcloudServiceRoot::initiateSyncIn);
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
  return true;
}

bool NextcloudServiceRoot::supportsCategoryAdding() const {
  return false;
}

void NextcloudServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, NextcloudFeed>(this);
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
    requestSyncIn();
  }
}

QString NextcloudServiceRoot::code() const {
  return NextcloudServiceEntryPoint().code();
}

NextcloudNetworkFactory* NextcloudServiceRoot::network() const {
  return m_network;
}

void NextcloudServiceRoot::requestSyncIn() {
  if (m_syncInRunning) {
    return;
  }

  ServiceRoot::requestSyncIn();

  QThreadPool::globalInstance()->start([this]() {
    try {
      auto* feed_cats = m_network->feedsCategories(networkProxy());
      m_network->obtainIcons(feed_cats->getSubTreeFeeds(), networkProxy());

      emit syncInFinished(feed_cats);
    }
    catch (const ApplicationException& ex) {
      emit syncInFinished(ex);
    }
  });
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
      QStringList article_ids;

      for (const Message& msg : messages) {
        article_ids.append(msg.m_customId);
      }

      auto res = network()->markMessagesStarred(key, article_ids, networkProxy());

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
  auto* feed_cats = m_network->feedsCategories(networkProxy());
  m_network->obtainIcons(feed_cats->getSubTreeFeeds(), networkProxy());

  return feed_cats;
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

  try {
    auto messages = network()->getMessages(feed->customNumericId(), networkProxy());
    return messages;
  }
  catch (const NetworkException& netEx) {
    throw FeedFetchException(Feed::Status::NetworkError, netEx.message());
  }
}

void NextcloudServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot add item"),
                          tr("Cannot add feed because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});

    return;
  }

  QScopedPointer<FormNextcloudFeedDetails> form_pointer(new FormNextcloudFeedDetails(this,
                                                                                     selected_item,
                                                                                     url,
                                                                                     qApp->mainFormWidget()));

  form_pointer->addEditFeed<NextcloudFeed>();
  qApp->feedUpdateLock()->unlock();
}