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
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/owncloud/owncloudrecyclebin.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"

#include <QSqlQuery>
#include <QSqlError>


OwnCloudServiceRoot::OwnCloudServiceRoot(RootItem *parent)
  : ServiceRoot(parent), m_recycleBin(new OwnCloudRecycleBin(this)), m_network(new OwnCloudNetworkFactory()) {
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
  return false;
}

bool OwnCloudServiceRoot::deleteViaGui() {
  return false;
}

bool OwnCloudServiceRoot::supportsFeedAdding() const {
  // TODO: TODO
  return false;
}

bool OwnCloudServiceRoot::supportsCategoryAdding() const {
  // TODO: TODO
  return false;
}

QList<QAction*> OwnCloudServiceRoot::addItemMenu() {
  // TODO: TODO
  return QList<QAction*>();
}

QList<QAction*> OwnCloudServiceRoot::serviceMenu() {
  // TODO: TODO
  return QList<QAction*>();
}

RecycleBin *OwnCloudServiceRoot::recycleBin() const {
  // TODO: TODO
  return NULL;
}

void OwnCloudServiceRoot::start(bool freshly_activated) {
  // TODO: TODO
  //loadFromDatabase();

  if (childCount() == 1 && child(0)->kind() == RootItemKind::Bin) {
    syncIn();
  }
}

void OwnCloudServiceRoot::stop() {
  // TODO: TODO
}

QString OwnCloudServiceRoot::code() const {
  return SERVICE_CODE_OWNCLOUD;
}

bool OwnCloudServiceRoot::loadMessagesForItem(RootItem *item, QSqlTableModel *model) {
  // TODO: TODO
  return false;
}

OwnCloudNetworkFactory *OwnCloudServiceRoot::network() const {
  return m_network;
}

void OwnCloudServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->authUsername() + QL1S("@") + host);
}

void OwnCloudServiceRoot::saveAccountDataToDatabase() {
  // TODO: TODO

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
    query.bindValue(QSL(":type"), SERVICE_CODE_OWNCLOUD);

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
  // TODO: TODO
}

void OwnCloudServiceRoot::addNewCategory() {
  // TODO: TODO
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
