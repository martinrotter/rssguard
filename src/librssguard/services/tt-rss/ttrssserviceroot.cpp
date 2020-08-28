// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formeditttrssaccount.h"
#include "services/tt-rss/gui/formttrssfeeddetails.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssfeed.h"
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

void TtRssServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile(accountId());

  if (childCount() <= 2) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  saveCacheToFile(accountId());

  m_network->logout();
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

  form_pointer->execForEdit(this);
  return true;
}

bool TtRssServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  // Remove extra entry in "Tiny Tiny RSS accounts list" and then delete
  // all the categories/feeds and messages.
  if (DatabaseQueries::deleteTtRssAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::supportsFeedAdding() const {
  return true;
}

bool TtRssServiceRoot::supportsCategoryAdding() const {
  return false;
}

void TtRssServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
  Q_UNUSED(selected_item)

  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot add item"),
                         tr("Cannot add feed because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);

    // Thus, cannot delete and quit the method.
    return;
  }

  QScopedPointer<FormTtRssFeedDetails> form_pointer(new FormTtRssFeedDetails(this, qApp->mainFormWidget()));

  form_pointer->addEditFeed(nullptr, selected_item, url);
  qApp->feedUpdateLock()->unlock();
}

bool TtRssServiceRoot::canBeEdited() const {
  return true;
}

bool TtRssServiceRoot::canBeDeleted() const {
  return true;
}

void TtRssServiceRoot::saveAllCachedData(bool async) {
  QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>> msgCache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msgCache.first);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      network()->updateArticles(ids,
                                UpdateArticle::OperatingField::Unread,
                                key == RootItem::ReadStatus::Unread
                                ? UpdateArticle::Mode::SetToTrue
                                : UpdateArticle::Mode::SetToFalse,
                                async);
    }
  }

  QMapIterator<RootItem::Importance, QList<Message>> j(msgCache.second);

  // Save the actual data important/not important.
  while (j.hasNext()) {
    j.next();
    auto key = j.key();
    QList<Message> messages = j.value();

    if (!messages.isEmpty()) {
      QStringList ids = customIDsOfMessages(messages);

      network()->updateArticles(ids,
                                UpdateArticle::OperatingField::Starred,
                                key == RootItem::Importance::Important
                                ? UpdateArticle::Mode::SetToTrue
                                : UpdateArticle::Mode::SetToFalse,
                                async);
    }
  }
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

void TtRssServiceRoot::saveAccountDataToDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (accountId() != NO_PARENT_CATEGORY) {
    // We are overwritting previously saved data.
    if (DatabaseQueries::overwriteTtRssAccount(database, m_network->username(), m_network->password(),
                                               m_network->authIsUsed(), m_network->authUsername(),
                                               m_network->authPassword(), m_network->url(),
                                               m_network->forceServerSideUpdate(), m_network->downloadOnlyUnreadMessages(),
                                               accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    bool saved;
    int id_to_assign = DatabaseQueries::createAccount(database, code(), &saved);

    if (saved) {
      if (DatabaseQueries::createTtRssAccount(database, id_to_assign, m_network->username(),
                                              m_network->password(), m_network->authIsUsed(),
                                              m_network->authUsername(), m_network->authPassword(),
                                              m_network->url(), m_network->forceServerSideUpdate(),
                                              m_network->downloadOnlyUnreadMessages())) {
        setId(id_to_assign);
        setAccountId(id_to_assign);
        updateTitle();
      }
    }
  }
}

void TtRssServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<TtRssFeed>(database, qApp->feedReader()->messageFilters(), accountId());

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(recycleBin());
  appendChild(importantNode());
  updateCounts(true);
}

void TtRssServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->username() + QSL(" (Tiny Tiny RSS)"));
}

RootItem* TtRssServiceRoot::obtainNewTreeForSyncIn() const {
  TtRssGetFeedsCategoriesResponse feed_cats_response = m_network->getFeedsCategories();

  if (m_network->lastError() == QNetworkReply::NoError) {
    return feed_cats_response.feedsCategories(true, m_network->url());
  }
  else {
    return nullptr;
  }
}
