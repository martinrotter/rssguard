// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/owncloud/owncloudserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/iconfactory.h"
#include "gui/dialogs/formmain.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/owncloud/owncloudrecyclebin.h"
#include "services/owncloud/owncloudfeed.h"
#include "services/owncloud/owncloudcategory.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"

#include <QSqlQuery>
#include <QSqlError>


OwnCloudServiceRoot::OwnCloudServiceRoot(RootItem *parent)
  : ServiceRoot(parent), m_recycleBin(new OwnCloudRecycleBin(this)),
    m_actionSyncIn(NULL), m_serviceMenu(QList<QAction*>()), m_network(new OwnCloudNetworkFactory()) {
  setIcon(OwnCloudServiceEntryPoint().icon());
}

OwnCloudServiceRoot::~OwnCloudServiceRoot() {
  delete m_network;
}

bool OwnCloudServiceRoot::canBeEdited() const {
  return true;
}

bool OwnCloudServiceRoot::canBeDeleted() const {
  return true;
}

bool OwnCloudServiceRoot::editViaGui() {
  QScopedPointer<FormEditOwnCloudAccount> form_pointer(new FormEditOwnCloudAccount(qApp->mainForm()));
  form_pointer.data()->execForEdit(this);

  return true;
}

bool OwnCloudServiceRoot::deleteViaGui() {
  QSqlDatabase connection = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(connection);

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM OwnCloudAccounts WHERE id = :id;"));
  query.bindValue(QSL(":id"), accountId());

  if (query.exec()) {
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

QList<QAction*> OwnCloudServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("item-sync")), tr("Sync in"), this);

    connect(m_actionSyncIn, SIGNAL(triggered()), this, SLOT(syncIn()));
    m_serviceMenu.append(m_actionSyncIn);
  }

  return m_serviceMenu;
}

RecycleBin *OwnCloudServiceRoot::recycleBin() const {
  return m_recycleBin;
}

void OwnCloudServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)

  loadFromDatabase();

  if (qApp->isFirstRun(QSL("3.1.1")) || (childCount() == 1 && child(0)->kind() == RootItemKind::Bin)) {
    syncIn();
  }
}

void OwnCloudServiceRoot::stop() {
}

QString OwnCloudServiceRoot::code() const {
  return OwnCloudServiceEntryPoint().code();
}

OwnCloudNetworkFactory *OwnCloudServiceRoot::network() const {
  return m_network;
}

bool OwnCloudServiceRoot::onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages,
                                                  RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  QNetworkReply::NetworkError reply = network()->markMessagesRead(read, customIDsOfMessages(messages));
  return reply == QNetworkReply::NoError;
}

bool OwnCloudServiceRoot::onBeforeSwitchMessageImportance(RootItem *selected_item,
                                                          const QList<ImportanceChange> &changes) {
  Q_UNUSED(selected_item)

  // Now, we need to separate the changes because of ownCloud API limitations.
  QStringList mark_starred_feed_ids, mark_starred_guid_hashes;
  QStringList mark_unstarred_feed_ids, mark_unstarred_guid_hashes;

  foreach (const ImportanceChange &pair, changes) {
    if (pair.second == RootItem::Important) {
      mark_starred_feed_ids.append(pair.first.m_feedId);
      mark_starred_guid_hashes.append(pair.first.m_customHash);
    }
    else {
      mark_unstarred_feed_ids.append(pair.first.m_feedId);
      mark_unstarred_guid_hashes.append(pair.first.m_customHash);
    }
  }

  // OK, now perform the online update itself.

  if (!mark_starred_feed_ids.isEmpty()) {
    if (network()->markMessagesStarred(RootItem::Important, mark_starred_feed_ids, mark_starred_guid_hashes) !=
        QNetworkReply::NoError) {
      return false;
    }
  }

  if (!mark_unstarred_feed_ids.isEmpty()) {
    if (network()->markMessagesStarred(RootItem::NotImportant, mark_unstarred_feed_ids, mark_unstarred_guid_hashes) !=
        QNetworkReply::NoError) {
      return false;
    }
  }

  return true;
}

void OwnCloudServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->authUsername() + QL1S("@") + host);
}

void OwnCloudServiceRoot::saveAccountDataToDatabase() {
  if (accountId() != NO_PARENT_CATEGORY) {
    // We are overwritting previously saved data.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query(database);

    query.prepare("UPDATE OwnCloudAccounts "
                  "SET username = :username, password = :password, url = :url, force_update = :force_update "
                  "WHERE id = :id;");
    query.bindValue(QSL(":username"), m_network->authUsername());
    query.bindValue(QSL(":password"), TextFactory::encrypt(m_network->authPassword()));
    query.bindValue(QSL(":url"), m_network->url());
    query.bindValue(QSL(":force_update"), (int) m_network->forceServerSideUpdate());
    query.bindValue(QSL(":id"), accountId());

    if (query.exec()) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
    else {
      qWarning("OwnCloud: Updating account failed: '%s'.", qPrintable(query.lastError().text()));
    }
  }
  else {
    // We are probably saving newly added account.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query(database);

    // First obtain the ID, which can be assigned to this new account.
    if (!query.exec("SELECT max(id) FROM Accounts;") || !query.next()) {
      qWarning("OwnCloud: Getting max ID from Accounts table failed: '%s'.", qPrintable(query.lastError().text()));
      return;
    }

    int id_to_assign = query.value(0).toInt() + 1;
    bool saved = true;

    query.prepare(QSL("INSERT INTO Accounts (id, type) VALUES (:id, :type);"));
    query.bindValue(QSL(":id"), id_to_assign);
    query.bindValue(QSL(":type"), code());

    saved &= query.exec();

    query.prepare("INSERT INTO OwnCloudAccounts (id, username, password, url, force_update) "
                  "VALUES (:id, :username, :password, :url, :force_update);");
    query.bindValue(QSL(":id"), id_to_assign);
    query.bindValue(QSL(":username"), m_network->authUsername());
    query.bindValue(QSL(":password"), TextFactory::encrypt(m_network->authPassword()));
    query.bindValue(QSL(":url"), m_network->url());
    query.bindValue(QSL(":force_update"), (int) m_network->forceServerSideUpdate());

    saved &= query.exec();

    if (saved) {
      setId(id_to_assign);
      setAccountId(id_to_assign);
      updateTitle();
    }
    else {
      qWarning("OwnCloud: Saving of new account failed: '%s'.", qPrintable(query.lastError().text()));
    }
  }
}

void OwnCloudServiceRoot::addNewFeed(const QString &url) {
}

void OwnCloudServiceRoot::addNewCategory() {
}

void OwnCloudServiceRoot::syncIn() {
  QIcon original_icon = icon();

  setIcon(qApp->icons()->fromTheme(QSL("item-sync")));
  itemChanged(QList<RootItem*>() << this);

  OwnCloudGetFeedsCategoriesResponse feed_cats_response = m_network->feedsCategories();

  if (m_network->lastError() == QNetworkReply::NoError) {
    RootItem *new_tree = feed_cats_response.feedsCategories(true);

    // Purge old data from SQL and clean all model items.
    requestItemExpandStateSave(this);
    removeOldFeedTree(false);
    cleanAllItems();

    // Model is clean, now store new tree into DB and
    // set primary IDs of the items.
    storeNewFeedTree(new_tree);

    // We have new feed, some feeds were maybe removed,
    // so remove left over messages.
    removeLeftOverMessages();

    foreach (RootItem *top_level_item, new_tree->childItems()) {
      top_level_item->setParent(NULL);
      requestItemReassignment(top_level_item, this);
    }

    updateCounts(true);

    new_tree->clearChildren();
    new_tree->deleteLater();

    QList<RootItem*> all_items = getSubTree();

    itemChanged(all_items);
    requestReloadMessageList(true);

    // Now we must refresh expand states.
    QList<RootItem*> items_to_expand;

    foreach (RootItem *item, all_items) {
      if (qApp->settings()->value(GROUP(CategoriesExpandStates), item->hashCode(), item->childCount() > 0).toBool()) {
        items_to_expand.append(item);
      }
    }

    requestItemExpand(items_to_expand, true);
  }

  setIcon(original_icon);
  itemChanged(QList<RootItem*>() << this);
}

void OwnCloudServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  Assignment categories;
  Assignment feeds;

  // Obtain data for categories from the database.
  QSqlQuery query_categories(database);
  query_categories.setForwardOnly(true);
  query_categories.prepare(QSL("SELECT * FROM Categories WHERE account_id = :account_id;"));
  query_categories.bindValue(QSL(":account_id"), accountId());

  if (!query_categories.exec()) {
    qFatal("Query for obtaining categories failed. Error message: '%s'.", qPrintable(query_categories.lastError().text()));
  }

  while (query_categories.next()) {
    AssignmentItem pair;
    pair.first = query_categories.value(CAT_DB_PARENT_ID_INDEX).toInt();
    pair.second = new OwnCloudCategory(query_categories.record());

    categories << pair;
  }

  // All categories are now loaded.
  QSqlQuery query_feeds(database);
  query_feeds.setForwardOnly(true);
  query_feeds.prepare(QSL("SELECT * FROM Feeds WHERE account_id = :account_id;"));
  query_feeds.bindValue(QSL(":account_id"), accountId());

  if (!query_feeds.exec()) {
    qFatal("Query for obtaining feeds failed. Error message: '%s'.", qPrintable(query_feeds.lastError().text()));
  }

  while (query_feeds.next()) {
    AssignmentItem pair;
    pair.first = query_feeds.value(FDS_DB_CATEGORY_INDEX).toInt();
    pair.second = new OwnCloudFeed(query_feeds.record());

    feeds << pair;
  }

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(m_recycleBin);
  updateCounts(true);
}
