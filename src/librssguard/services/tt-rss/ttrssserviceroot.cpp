// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssserviceroot.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "exceptions/feedfetchexception.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/labelsnode.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formeditttrssaccount.h"
#include "services/tt-rss/gui/formttrssfeeddetails.h"
#include "services/tt-rss/gui/formttrssnote.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <QPair>
#include <QSqlTableModel>

TtRssServiceRoot::TtRssServiceRoot(RootItem* parent) : ServiceRoot(parent), m_network(new TtRssNetworkFactory()) {
  setIcon(TtRssServiceEntryPoint().icon());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  delete m_network;
}

ServiceRoot::LabelOperation TtRssServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation::Synchronised;
}

void TtRssServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, TtRssFeed>(this);
    loadCacheFromFile();

    auto lbls = labelsNode()->labels();

    boolinq::from(lbls).for_each([](Label* lbl) {
      if (lbl->customNumericId() == TTRSS_PUBLISHED_LABEL_ID) {
        lbl->setKeepOnTop(true);
      }
    });

    boolinq::from(childItems()).for_each([](RootItem* child) {
      if (child->kind() == RootItem::Kind::Feed && child->customNumericId() == TTRSS_PUBLISHED_FEED_ID) {
        child->setKeepOnTop(true);
      }
    });
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  m_network->logout(networkProxy());
  qDebugNN << LOGSEC_TTRSS << "Stopping Tiny Tiny RSS account, logging out with result"
           << QUOTE_W_SPACE_DOT(m_network->lastError());
}

QString TtRssServiceRoot::code() const {
  return TtRssServiceEntryPoint().code();
}

bool TtRssServiceRoot::isSyncable() const {
  return true;
}

bool TtRssServiceRoot::editViaGui() {
  QScopedPointer<FormEditTtRssAccount> form_pointer(new FormEditTtRssAccount(qApp->mainFormWidget()));

  form_pointer->addEditAccount(this);
  return true;
}

bool TtRssServiceRoot::supportsFeedAdding() const {
  return true;
}

bool TtRssServiceRoot::supportsCategoryAdding() const {
  return false;
}

void TtRssServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
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

  QScopedPointer<FormTtRssFeedDetails> form_pointer(new FormTtRssFeedDetails(this,
                                                                             selected_item,
                                                                             url,
                                                                             qApp->mainFormWidget()));

  form_pointer->addEditFeed<TtRssFeed>();
  qApp->feedUpdateLock()->unlock();
}

bool TtRssServiceRoot::canBeEdited() const {
  return true;
}

void TtRssServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      auto res = network()->updateArticles(ids,
                                           UpdateArticle::OperatingField::Unread,
                                           key == RootItem::ReadStatus::Unread ? UpdateArticle::Mode::SetToTrue
                                                                               : UpdateArticle::Mode::SetToFalse,
                                           networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
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
      QStringList ids = customIDsOfMessages(messages);
      auto res = network()->updateArticles(ids,
                                           UpdateArticle::OperatingField::Starred,
                                           key == RootItem::Importance::Important ? UpdateArticle::Mode::SetToTrue
                                                                                  : UpdateArticle::Mode::SetToFalse,
                                           networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
        addMessageStatesToCache(messages, key);
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
      TtRssResponse res;

      if (label_custom_id.toInt() == TTRSS_PUBLISHED_LABEL_ID) {
        // "published" label must be added in other method.
        res = network()->updateArticles(messages,
                                        UpdateArticle::OperatingField::Published,
                                        UpdateArticle::Mode::SetToTrue,
                                        networkProxy());
      }
      else {
        res = network()->setArticleLabel(messages, label_custom_id, true, networkProxy());
      }

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
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
      TtRssResponse res;

      if (label_custom_id.toInt() == TTRSS_PUBLISHED_LABEL_ID) {
        // "published" label must be removed in other method.
        res = network()->updateArticles(messages,
                                        UpdateArticle::OperatingField::Published,
                                        UpdateArticle::Mode::SetToFalse,
                                        networkProxy());
      }
      else {
        res = network()->setArticleLabel(messages, label_custom_id, false, networkProxy());
      }

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
        addLabelsAssignmentsToCache(messages, label_custom_id, false);
      }
    }
  }
}

QVariantHash TtRssServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data[QSL("username")] = m_network->username();
  data[QSL("password")] = TextFactory::encrypt(m_network->password());
  data[QSL("auth_protected")] = m_network->authIsUsed();
  data[QSL("auth_username")] = m_network->authUsername();
  data[QSL("auth_password")] = TextFactory::encrypt(m_network->authPassword());
  data[QSL("url")] = m_network->url();
  data[QSL("force_update")] = m_network->forceServerSideUpdate();
  data[QSL("batch_size")] = m_network->batchSize();
  data[QSL("download_only_unread")] = m_network->downloadOnlyUnreadMessages();
  data[QSL("intelligent_synchronization")] = m_network->intelligentSynchronization();

  return data;
}

void TtRssServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setUsername(data[QSL("username")].toString());
  m_network->setPassword(TextFactory::decrypt(data[QSL("password")].toString()));
  m_network->setAuthIsUsed(data[QSL("auth_protected")].toBool());
  m_network->setAuthUsername(data[QSL("auth_username")].toString());
  m_network->setAuthPassword(TextFactory::decrypt(data[QSL("auth_password")].toString()));
  m_network->setUrl(data[QSL("url")].toString());
  m_network->setForceServerSideUpdate(data[QSL("force_update")].toBool());
  m_network->setBatchSize(data[QSL("batch_size")].toInt());
  m_network->setDownloadOnlyUnreadMessages(data[QSL("download_only_unread")].toBool());
  m_network->setIntelligentSynchronization(data[QSL("intelligent_synchronization")].toBool());
}

QList<Message> TtRssServiceRoot::obtainNewMessages(Feed* feed,
                                                   const QHash<ServiceRoot::BagOfMessages, QStringList>&
                                                     stated_messages,
                                                   const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(tagged_messages)

  if (m_network->intelligentSynchronization()) {
    return obtainMessagesIntelligently(feed, stated_messages);
  }
  else {
    return obtainMessagesViaHeadlines(feed);
  }
}

QList<Message> TtRssServiceRoot::obtainMessagesIntelligently(Feed* feed,
                                                             const QHash<BagOfMessages, QStringList>& stated_messages) {
  // 1. Get unread IDs for a feed.
  // 2. Get read IDs for a feed.
  // 3. Get starred IDs for a feed.
  // 4. Determine IDs needed to download.
  // 5. Download needed articles.
  const QStringList remote_all_ids_list =
    m_network->downloadOnlyUnreadMessages()
      ? QStringList()
      : m_network->getCompactHeadlines(feed->customNumericId(), 1000000, 0, QSL("all_articles"), networkProxy()).ids();
  const QStringList remote_unread_ids_list =
    m_network->getCompactHeadlines(feed->customNumericId(), 1000000, 0, QSL("unread"), networkProxy()).ids();
  const QStringList remote_starred_ids_list =
    m_network->getCompactHeadlines(feed->customNumericId(), 1000000, 0, QSL("marked"), networkProxy()).ids();

  const QSet<QString> remote_all_ids = FROM_LIST_TO_SET(QSet<QString>, remote_all_ids_list);

  // 1.
  auto local_unread_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Unread);
  const QSet<QString> remote_unread_ids = FROM_LIST_TO_SET(QSet<QString>, remote_unread_ids_list);
  const QSet<QString> local_unread_ids = FROM_LIST_TO_SET(QSet<QString>, local_unread_ids_list);

  // 2.
  const auto local_read_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Read);
  const QSet<QString> remote_read_ids = remote_all_ids - remote_unread_ids;
  const QSet<QString> local_read_ids = FROM_LIST_TO_SET(QSet<QString>, local_read_ids_list);

  // 3.
  const auto local_starred_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Starred);
  const QSet<QString> remote_starred_ids = FROM_LIST_TO_SET(QSet<QString>, remote_starred_ids_list);
  const QSet<QString> local_starred_ids = FROM_LIST_TO_SET(QSet<QString>, local_starred_ids_list);

  // 4.
  QSet<QString> to_download;

  if (!m_network->downloadOnlyUnreadMessages()) {
    to_download += remote_all_ids - local_read_ids - local_unread_ids;
  }
  else {
    to_download += remote_unread_ids - local_read_ids - local_unread_ids;
  }

  auto moved_read = local_read_ids & remote_unread_ids;

  to_download += moved_read;

  if (!m_network->downloadOnlyUnreadMessages()) {
    auto moved_unread = local_unread_ids & remote_read_ids;

    to_download += moved_unread;
  }

  auto moved_starred = (local_starred_ids + remote_starred_ids) - (local_starred_ids & remote_starred_ids);

  to_download += moved_starred;

  // 5.
  auto msgs = m_network->getArticle(to_download.values(), networkProxy());

  return msgs.messages(this);
}

QList<Message> TtRssServiceRoot::obtainMessagesViaHeadlines(Feed* feed) {
  QList<Message> messages;
  int newly_added_messages = 0;
  int limit = network()->batchSize() <= 0 ? TTRSS_MAX_MESSAGES : network()->batchSize();
  int skip = 0;

  do {
    TtRssGetHeadlinesResponse headlines = network()->getHeadlines(feed->customNumericId(),
                                                                  limit,
                                                                  skip,
                                                                  true,
                                                                  true,
                                                                  false,
                                                                  network()->downloadOnlyUnreadMessages(),
                                                                  networkProxy());

    if (network()->lastError() != QNetworkReply::NetworkError::NoError) {
      throw FeedFetchException(Feed::Status::NetworkError, headlines.error());
    }
    else {
      QList<Message> new_messages = headlines.messages(this);

      messages << new_messages;
      newly_added_messages = new_messages.size();
      skip += newly_added_messages;
    }
  }
  while (newly_added_messages > 0 && (network()->batchSize() <= 0 || messages.size() < network()->batchSize()));

  return messages;
}

QString TtRssServiceRoot::additionalTooltip() const {
  return ServiceRoot::additionalTooltip() + QSL("\n") +
         tr("Username: %1\nServer: %2\n"
            "Last error: %3\nLast login on: %4")
           .arg(m_network->username(),
                m_network->url(),
                NetworkFactory::networkErrorText(m_network->lastError()),
                m_network->lastLoginTime().isValid()
                  ? QLocale().toString(m_network->lastLoginTime(), QLocale::FormatType::ShortFormat)
                  : QSL("-"));
}

TtRssNetworkFactory* TtRssServiceRoot::network() const {
  return m_network;
}

void TtRssServiceRoot::shareToPublished() {
  FormTtRssNote(this).exec();
}

void TtRssServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(TextFactory::extractUsernameFromEmail(m_network->username()) + QSL(" (Tiny Tiny RSS)"));
}

RootItem* TtRssServiceRoot::obtainNewTreeForSyncIn() const {
  TtRssGetFeedsCategoriesResponse feed_cats = m_network->getFeedsCategories(networkProxy());
  TtRssGetLabelsResponse labels = m_network->getLabels(networkProxy());

  auto lst_error = m_network->lastError();

  if (lst_error == QNetworkReply::NoError) {
    auto* tree = feed_cats.feedsCategories(m_network, true, networkProxy(), m_network->url());
    auto* lblroot = new LabelsNode(tree);

    lblroot->setChildItems(labels.labels());
    tree->appendChild(lblroot);

    return tree;
  }
  else {
    throw NetworkException(lst_error, tr("cannot get list of feeds, network error '%1'").arg(lst_error));
  }
}

bool TtRssServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return m_network->intelligentSynchronization();
}
