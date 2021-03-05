// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "database/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formeditttrssaccount.h"
#include "services/tt-rss/gui/formttrssfeeddetails.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <QClipboard>
#include <QPair>
#include <QSqlTableModel>

TtRssServiceRoot::TtRssServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new TtRssNetworkFactory()) {
  setIcon(TtRssServiceEntryPoint().icon());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  delete m_network;
}

ServiceRoot::LabelOperation TtRssServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation(0);
}

void TtRssServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadFromDatabase<Category, TtRssFeed>(this);
    loadCacheFromFile();
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  m_network->logout(networkProxy());
  qDebugNN << LOGSEC_TTRSS
           << "Stopping Tiny Tiny RSS account, logging out with result"
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
    qApp->showGuiMessage(tr("Cannot add item"),
                         tr("Cannot add feed because another critical operation is ongoing."),
                         QSystemTrayIcon::MessageIcon::Warning,
                         qApp->mainFormWidget(),
                         true);

    return;
  }

  QScopedPointer<FormTtRssFeedDetails> form_pointer(new FormTtRssFeedDetails(this, selected_item, url, qApp->mainFormWidget()));

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
                                           key == RootItem::ReadStatus::Unread
                                           ? UpdateArticle::Mode::SetToTrue
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
                                           key == RootItem::Importance::Important
                                           ? UpdateArticle::Mode::SetToTrue
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
      auto res = network()->setArticleLabel(messages, label_custom_id, true, networkProxy());

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
      auto res = network()->setArticleLabel(messages, label_custom_id, false, networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
        addLabelsAssignmentsToCache(messages, label_custom_id, false);
      }
    }
  }
}

QVariantHash TtRssServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data["username"] = m_network->username();
  data["password"] = TextFactory::encrypt(m_network->password());
  data["auth_protected"] = m_network->authIsUsed();
  data["auth_username"] = m_network->authUsername();
  data["auth_password"] = TextFactory::encrypt(m_network->authPassword());
  data["url"] = m_network->url();
  data["force_update"] = m_network->forceServerSideUpdate();
  data["download_only_unread"] = m_network->downloadOnlyUnreadMessages();

  return data;
}

void TtRssServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setUsername( data["username"].toString());
  m_network->setPassword(TextFactory::decrypt(data["password"].toString()));
  m_network->setAuthIsUsed(data["auth_protected"].toBool());
  m_network->setAuthUsername(data["auth_username"].toString());
  m_network->setAuthPassword(TextFactory::decrypt(data["auth_password"].toString()));
  m_network->setUrl(data["url"].toString());
  m_network->setForceServerSideUpdate(data["force_update"].toBool());
  m_network->setDownloadOnlyUnreadMessages(data["download_only_unread"].toBool());
}

QList<Message> TtRssServiceRoot::obtainNewMessages(const QList<Feed*>& feeds, bool* error_during_obtaining) {
  QList<Message> messages;

  for (Feed* feed : feeds) {
    int newly_added_messages = 0;
    int limit = TTRSS_MAX_MESSAGES;
    int skip = 0;

    do {
      TtRssGetHeadlinesResponse headlines = network()->getHeadlines(customId().toInt(), limit, skip,
                                                                    true, true, false,
                                                                    network()->downloadOnlyUnreadMessages(),
                                                                    networkProxy());

      if (network()->lastError() != QNetworkReply::NetworkError::NoError) {
        feed->setStatus(Feed::Status::NetworkError);
        *error_during_obtaining = true;
        itemChanged(QList<RootItem*>() << this);
        continue;
      }
      else {
        QList<Message> new_messages = headlines.messages(getParentServiceRoot());

        messages << new_messages;
        newly_added_messages = new_messages.size();
        skip += newly_added_messages;
      }
    }
    while (newly_added_messages > 0);
  }

  return messages;
}

QString TtRssServiceRoot::additionalTooltip() const {
  return tr("Username: %1\nServer: %2\n"
            "Last error: %3\nLast login on: %4").arg(m_network->username(),
                                                     m_network->url(),
                                                     NetworkFactory::networkErrorText(m_network->lastError()),
                                                     m_network->lastLoginTime().isValid()
                                                     ? QLocale().toString(m_network->lastLoginTime(), QLocale::FormatType::ShortFormat)
                                                     : QSL("-"));
}

TtRssNetworkFactory* TtRssServiceRoot::network() const {
  return m_network;
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

  if (m_network->lastError() == QNetworkReply::NoError) {
    auto* tree = feed_cats.feedsCategories(true, m_network->url());
    auto* lblroot = new LabelsNode(tree);

    lblroot->setChildItems(labels.labels());
    tree->appendChild(lblroot);

    return tree;
  }
  else {
    return nullptr;
  }
}
