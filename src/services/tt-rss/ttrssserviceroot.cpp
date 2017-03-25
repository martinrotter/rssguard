// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/databasequeries.h"
#include "network-web/networkfactory.h"
#include "miscellaneous/iconfactory.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssrecyclebin.h"
#include "services/tt-rss/ttrsscategory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/gui/formeditaccount.h"
#include "services/tt-rss/gui/formttrssfeeddetails.h"

#include <QSqlTableModel>
#include <QPair>
#include <QClipboard>


TtRssServiceRoot::TtRssServiceRoot(RootItem *parent)
  : ServiceRoot(parent), m_recycleBin(new TtRssRecycleBin(this)),
    m_actionSyncIn(nullptr), m_serviceMenu(QList<QAction*>()), m_network(new TtRssNetworkFactory()) {
  setIcon(TtRssServiceEntryPoint().icon());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  delete m_network;
}

void TtRssServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)

  loadFromDatabase();

  if (qApp->isFirstRun(QSL("3.1.1")) || (childCount() == 1 && child(0)->kind() == RootItemKind::Bin)) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  m_network->logout();
  qDebug("Stopping Tiny Tiny RSS account, logging out with result '%d'.", (int) m_network->lastError());
}

QString TtRssServiceRoot::code() const {
  return TtRssServiceEntryPoint().code();
}

bool TtRssServiceRoot::editViaGui() {
  QScopedPointer<FormEditAccount> form_pointer(new FormEditAccount(qApp->mainFormWidget()));
  form_pointer.data()->execForEdit(this);

  return true;
}

bool TtRssServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  // Remove extra entry in "Tiny Tiny RSS accounts list" and then delete
  // all the categories/feeds and messages.
  if (DatabaseQueries::deleteTtRssAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  QStringList ids = customIDSOfMessagesForItem(this);
  TtRssUpdateArticleResponse response = m_network->updateArticles(ids, UpdateArticle::Unread,
                                                                  status == RootItem::Unread ?
                                                                    UpdateArticle::SetToTrue :
                                                                    UpdateArticle::SetToFalse);

  if (m_network->lastError() != QNetworkReply::NoError || response.updateStatus()  != STATUS_OK) {
    return false;
  }
  else {
    return ServiceRoot::markAsReadUnread(status);
  }
}

bool TtRssServiceRoot::supportsFeedAdding() const {
  return true;
}

bool TtRssServiceRoot::supportsCategoryAdding() const {
  return false;
}

void TtRssServiceRoot::addNewFeed(const QString &url) {
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

  form_pointer.data()->exec(nullptr, this, url);
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

QVariant TtRssServiceRoot::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return tr("Tiny Tiny RSS\n\nAccount ID: %3\nUsername: %1\nServer: %2\n"
                  "Last error: %4\nLast login on: %5").arg(m_network->username(),
                                                           m_network->url(),
                                                           QString::number(accountId()),
                                                           NetworkFactory::networkErrorText(m_network->lastError()),
                                                           m_network->lastLoginTime().isValid() ?
                                                             m_network->lastLoginTime().toString(Qt::DefaultLocaleShortDate) :
                                                             QSL("-"));
      }
      else {
        return ServiceRoot::data(column, role);
      }

    default:
      return ServiceRoot::data(column, role);
  }
}

RecycleBin *TtRssServiceRoot::recycleBin() const {
  return m_recycleBin;
}

QList<QAction*> TtRssServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Sync in"), this);

    connect(m_actionSyncIn, &QAction::triggered, this, &TtRssServiceRoot::syncIn);
    m_serviceMenu.append(m_actionSyncIn);
  }

  return m_serviceMenu;
}

bool TtRssServiceRoot::onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  TtRssUpdateArticleResponse response = m_network->updateArticles(customIDsOfMessages(messages),
                                                                  UpdateArticle::Unread,
                                                                  read == RootItem::Unread ?
                                                                    UpdateArticle::SetToTrue :
                                                                    UpdateArticle::SetToFalse);

  if (m_network->lastError() == QNetworkReply::NoError && response.updateStatus() == STATUS_OK) {
    return true;
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::onBeforeSwitchMessageImportance(RootItem *selected_item, const QList<ImportanceChange> &changes) {
  Q_UNUSED(selected_item)

  // NOTE: We just toggle it here, because we know, that there is only
  // toggling of starred status supported by RSS Guard right now and
  // Tiny Tiny RSS API allows it, which is great.
  TtRssUpdateArticleResponse response = m_network->updateArticles(customIDsOfMessages(changes),
                                                                  UpdateArticle::Starred,
                                                                  UpdateArticle::Togggle);

  if (m_network->lastError() == QNetworkReply::NoError && response.updateStatus() == STATUS_OK) {
    return true;
  }
  else {
    return false;
  }
}

TtRssNetworkFactory *TtRssServiceRoot::network() const {
  return m_network;
}

void TtRssServiceRoot::saveAccountDataToDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

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
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  Assignment categories = DatabaseQueries::getTtRssCategories(database, accountId());
  Assignment feeds = DatabaseQueries::getTtRssFeeds(database, accountId());

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(m_recycleBin);
  updateCounts(true);
}

void TtRssServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->username() + QL1S("@") + host + QSL(" (Tiny Tiny RSS)"));
}

RootItem *TtRssServiceRoot::obtainNewTreeForSyncIn() const {
  TtRssGetFeedsCategoriesResponse feed_cats_response = m_network->getFeedsCategories();

  if (m_network->lastError() == QNetworkReply::NoError) {
    return feed_cats_response.feedsCategories(true, m_network->url());
  }
  else {
    return nullptr;
  }
}

QMap<int,QVariant> TtRssServiceRoot::storeCustomFeedsData() {
  QMap<int,QVariant> custom_data;

  foreach (const Feed *feed, getSubTreeFeeds()) {
    QVariantMap feed_custom_data;

    feed_custom_data.insert(QSL("auto_update_interval"), feed->autoUpdateInitialInterval());
    feed_custom_data.insert(QSL("auto_update_type"), feed->autoUpdateType());

    custom_data.insert(feed->customId(), feed_custom_data);
  }

  return custom_data;
}

void TtRssServiceRoot::restoreCustomFeedsData(const QMap<int,QVariant> &data, const QHash<int,Feed*> &feeds) {
  foreach (int custom_id, data.keys()) {
    if (feeds.contains(custom_id)) {
      Feed *feed = feeds.value(custom_id);
      QVariantMap feed_custom_data = data.value(custom_id).toMap();

      feed->setAutoUpdateInitialInterval(feed_custom_data.value(QSL("auto_update_interval")).toInt());
      feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(feed_custom_data.value(QSL("auto_update_type")).toInt()));
    }
  }
}
