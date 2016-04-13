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
#include "miscellaneous/databasequeries.h"
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
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::deleteOwnCloudAccount(database, accountId())) {
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
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (accountId() != NO_PARENT_CATEGORY) {
    if (DatabaseQueries::overwriteOwnCloudAccount(database, m_network->authUsername(),
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
      if (DatabaseQueries::createOwnCloudAccount(database, id_to_assign, m_network->authUsername(),
                                             m_network->authPassword(), m_network->url(),
                                             m_network->forceServerSideUpdate())) {
        setId(id_to_assign);
        setAccountId(id_to_assign);
        updateTitle();
      }
    }
  }
}

void OwnCloudServiceRoot::addNewFeed(const QString &url) {
}

void OwnCloudServiceRoot::addNewCategory() {
}

RootItem *OwnCloudServiceRoot::obtainNewTreeForSyncIn() const {
  OwnCloudGetFeedsCategoriesResponse feed_cats_response = m_network->feedsCategories();

  if (m_network->lastError() == QNetworkReply::NoError) {
    return feed_cats_response.feedsCategories(true);
  }
  else {
    return NULL;
  }
}

void OwnCloudServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  Assignment categories = DatabaseQueries::getOwnCloudCategories(database, accountId());
  Assignment feeds = DatabaseQueries::getOwnCloudFeeds(database, accountId());

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(m_recycleBin);
  updateCounts(true);
}
