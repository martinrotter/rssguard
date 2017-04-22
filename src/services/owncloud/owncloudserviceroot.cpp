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

#include "services/owncloud/owncloudserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/owncloud/owncloudrecyclebin.h"
#include "services/owncloud/owncloudfeed.h"
#include "services/owncloud/owncloudcategory.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"
#include "services/owncloud/gui/formowncloudfeeddetails.h"


OwnCloudServiceRoot::OwnCloudServiceRoot(RootItem *parent)
  : ServiceRoot(parent), m_cacheSaveMutex(new Mutex(QMutex::NonRecursive, this)), m_cachedStatesRead(QMap<RootItem::ReadStatus, QStringList>()),
    m_cachedStatesImportant(QMap<RootItem::Importance, QStringList>()), m_recycleBin(new OwnCloudRecycleBin(this)),
    m_actionSyncIn(nullptr), m_serviceMenu(QList<QAction*>()), m_network(new OwnCloudNetworkFactory()) {
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
  QScopedPointer<FormEditOwnCloudAccount> form_pointer(new FormEditOwnCloudAccount(qApp->mainFormWidget()));
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
  return true;
}

bool OwnCloudServiceRoot::supportsCategoryAdding() const {
  return false;
}

QList<QAction*> OwnCloudServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Sync in"), this);

    connect(m_actionSyncIn, &QAction::triggered, this, &OwnCloudServiceRoot::syncIn);
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

void OwnCloudServiceRoot::addMessageStatesToCache(const QStringList &ids_of_messages, RootItem::ReadStatus read) {
  m_cacheSaveMutex->lock();

  QStringList &list_act = m_cachedStatesRead[read];
  QStringList &list_other = m_cachedStatesRead[read == RootItem::Read ? RootItem::Unread : RootItem::Read];

  // Store changes, they will be sent to server later.
  list_act.append(ids_of_messages);

  QSet<QString> set_act = list_act.toSet();
  QSet<QString> set_other = list_other.toSet();

  // Now, we want to remove all IDS from list_other, which are contained in list.
  set_other -= set_act;

  list_act.clear(); list_act.append(set_act.toList());
  list_other.clear(); list_other.append(set_other.toList());

  m_cacheSaveMutex->unlock();
}

void OwnCloudServiceRoot::saveAllCachedData() {
  if (m_cachedStatesRead.isEmpty() && m_cachedStatesImportant.isEmpty()) {
    // No cached changes.
    return;
  }

  m_cacheSaveMutex->lock();

  // Make copy of changes.
  QMap<RootItem::ReadStatus, QStringList> cached_data_read = m_cachedStatesRead;
  cached_data_read.detach();

  QMap<RootItem::Importance, QStringList> cached_data_imp = m_cachedStatesImportant;
  cached_data_imp.detach();

  m_cachedStatesRead.clear();
  m_cachedStatesImportant.clear();

  m_cacheSaveMutex->unlock();

  QMapIterator<RootItem::ReadStatus, QStringList> i(cached_data_read);

  // Save the actual data.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      network()->markMessagesRead(key, ids);
    }
  }
}

bool OwnCloudServiceRoot::onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages,
                                                  RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  addMessageStatesToCache(customIDsOfMessages(messages), read);
  return true;
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

  setTitle(m_network->authUsername() + QL1S("@") + host + QSL(" (ownCloud News)"));
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

  QScopedPointer<FormOwnCloudFeedDetails> form_pointer(new FormOwnCloudFeedDetails(this, qApp->mainFormWidget()));

  form_pointer.data()->exec(nullptr, this, url);
  qApp->feedUpdateLock()->unlock();
}

void OwnCloudServiceRoot::addNewCategory() {
}

QMap<int,QVariant> OwnCloudServiceRoot::storeCustomFeedsData() {
  QMap<int,QVariant> custom_data;

  foreach (const Feed *feed, getSubTreeFeeds()) {
    QVariantMap feed_custom_data;

    feed_custom_data.insert(QSL("auto_update_interval"), feed->autoUpdateInitialInterval());
    feed_custom_data.insert(QSL("auto_update_type"), feed->autoUpdateType());

    custom_data.insert(feed->customId(), feed_custom_data);
  }

  return custom_data;
}

void OwnCloudServiceRoot::restoreCustomFeedsData(const QMap<int,QVariant> &data, const QHash<int,Feed*> &feeds) {
  foreach (int custom_id, data.keys()) {
    if (feeds.contains(custom_id)) {
      Feed *feed = feeds.value(custom_id);
      QVariantMap feed_custom_data = data.value(custom_id).toMap();

      feed->setAutoUpdateInitialInterval(feed_custom_data.value(QSL("auto_update_interval")).toInt());
      feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(feed_custom_data.value(QSL("auto_update_type")).toInt()));
    }
  }
}

RootItem *OwnCloudServiceRoot::obtainNewTreeForSyncIn() const {
  OwnCloudGetFeedsCategoriesResponse feed_cats_response = m_network->feedsCategories();

  if (m_network->lastError() == QNetworkReply::NoError) {
    return feed_cats_response.feedsCategories(true);
  }
  else {
    return nullptr;
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
