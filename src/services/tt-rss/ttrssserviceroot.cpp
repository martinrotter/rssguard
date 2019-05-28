// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
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
  : ServiceRoot(parent), m_actionSyncIn(nullptr), m_network(new TtRssNetworkFactory()) {
  setIcon(TtRssServiceEntryPoint().icon());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  delete m_network;
}

void TtRssServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile(accountId());

  if (qApp->isFirstRun(QSL("3.1.1")) || (childCount() == 1 && child(0)->kind() == RootItemKind::Bin)) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  saveCacheToFile(accountId());

  m_network->logout();
  qDebug("Stopping Tiny Tiny RSS account, logging out with result '%d'.", (int) m_network->lastError());
}

QString TtRssServiceRoot::code() const {
  return TtRssServiceEntryPoint().code();
}

bool TtRssServiceRoot::editViaGui() {
  QScopedPointer<FormEditTtRssAccount> form_pointer(new FormEditTtRssAccount(qApp->mainFormWidget()));
  form_pointer.data()->execForEdit(this);
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

void TtRssServiceRoot::addNewFeed(const QString& url) {
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
  form_pointer.data()->addEditFeed(nullptr, this, url);
  qApp->feedUpdateLock()->unlock();
}

void TtRssServiceRoot::addNewCategory() {
  // NOTE: Do nothing.
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
                                UpdateArticle::Unread,
                                key == RootItem::Unread ? UpdateArticle::SetToTrue : UpdateArticle::SetToFalse,
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
                                UpdateArticle::Starred,
                                key == RootItem::Important ? UpdateArticle::SetToTrue : UpdateArticle::SetToFalse,
                                async);
    }
  }
}

QList<QAction*> TtRssServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Sync in"), this);
    connect(m_actionSyncIn, &QAction::triggered, this, &TtRssServiceRoot::syncIn);
    m_serviceMenu.append(m_actionSyncIn);
  }

  return m_serviceMenu;
}

QString TtRssServiceRoot::additionalTooltip() const {
  return tr("Username: %1\nServer: %2\n"
            "Last error: %3\nLast login on: %4").arg(m_network->username(),
                                                     m_network->url(),
                                                     NetworkFactory::networkErrorText(m_network->lastError()),
                                                     m_network->lastLoginTime().isValid() ?
                                                     m_network->lastLoginTime().toString(Qt::DefaultLocaleShortDate) :
                                                     QSL("-"));
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
                                               m_network->forceServerSideUpdate(), accountId())) {
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
                                              m_network->url(), m_network->forceServerSideUpdate())) {
        setId(id_to_assign);
        setAccountId(id_to_assign);
        updateTitle();
      }
    }
  }
}

void TtRssServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories(database, accountId());
  Assignment feeds = DatabaseQueries::getTtRssFeeds(database, accountId());

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(recycleBin());
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
