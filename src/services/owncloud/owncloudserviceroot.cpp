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


OwnCloudServiceRoot::OwnCloudServiceRoot(RootItem* parent)
	: ServiceRoot(parent), CacheForServiceRoot(), m_recycleBin(new OwnCloudRecycleBin(this)),
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

RecycleBin* OwnCloudServiceRoot::recycleBin() const {
	return m_recycleBin;
}

void OwnCloudServiceRoot::start(bool freshly_activated) {
	Q_UNUSED(freshly_activated)
	loadFromDatabase();
  //loadCacheFromFile(accountId());

	if (qApp->isFirstRun(QSL("3.1.1")) || (childCount() == 1 && child(0)->kind() == RootItemKind::Bin)) {
		syncIn();
	}
}

void OwnCloudServiceRoot::stop() {
  //saveCacheToFile(accountId());
}

QString OwnCloudServiceRoot::code() const {
	return OwnCloudServiceEntryPoint().code();
}

bool OwnCloudServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
	addMessageStatesToCache(customIDSOfMessagesForItem(this), status);
	return ServiceRoot::markAsReadUnread(status);
}

OwnCloudNetworkFactory* OwnCloudServiceRoot::network() const {
	return m_network;
}

void OwnCloudServiceRoot::saveAllCachedData() {
	QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>> msgCache = takeMessageCache();
	QMapIterator<RootItem::ReadStatus, QStringList> i(msgCache.first);

	// Save the actual data read/unread.
	while (i.hasNext()) {
		i.next();
		auto key = i.key();
		QStringList ids = i.value();

		if (!ids.isEmpty()) {
			network()->markMessagesRead(key, ids);
		}
	}

	QMapIterator<RootItem::Importance, QList<Message>> j(msgCache.second);

	// Save the actual data important/not important.
	while (j.hasNext()) {
		j.next();
		auto key = j.key();
		QList<Message> messages = j.value();

		if (!messages.isEmpty()) {
			QStringList feed_ids, guid_hashes;

			foreach (const Message& msg, messages) {
				feed_ids.append(msg.m_feedId);
				guid_hashes.append(msg.m_customHash);
			}

			network()->markMessagesStarred(key, feed_ids, guid_hashes);
		}
	}
}

bool OwnCloudServiceRoot::onBeforeSetMessagesRead(RootItem* selected_item,
                                                  const QList<Message>& messages,
                                                  RootItem::ReadStatus read) {
	Q_UNUSED(selected_item)
	addMessageStatesToCache(customIDsOfMessages(messages), read);
	return true;
}

bool OwnCloudServiceRoot::onBeforeSwitchMessageImportance(RootItem* selected_item,
                                                          const QList<ImportanceChange>& changes) {
	Q_UNUSED(selected_item)
	// Now, we need to separate the changes because of ownCloud API limitations.
	QList<Message> mark_starred_msgs;
	QList<Message> mark_unstarred_msgs;

	foreach (const ImportanceChange& pair, changes) {
		if (pair.second == RootItem::Important) {
			mark_starred_msgs.append(pair.first);
		}
		else {
			mark_unstarred_msgs.append(pair.first);
		}
	}

	if (!mark_starred_msgs.isEmpty()) {
		addMessageStatesToCache(mark_starred_msgs, RootItem::Important);
	}

	if (!mark_unstarred_msgs.isEmpty()) {
		addMessageStatesToCache(mark_unstarred_msgs, RootItem::NotImportant);
	}

	return true;
}

void OwnCloudServiceRoot::updateTitle() {
	QString host = QUrl(m_network->url()).host();

	if (host.isEmpty()) {
		host = m_network->url();
	}

	setTitle(m_network->authUsername() + QL1S("@") + host + QSL(" (Nextcloud News)"));
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

void OwnCloudServiceRoot::addNewFeed(const QString& url) {
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
	form_pointer.data()->addEditFeed(nullptr, this, url);
	qApp->feedUpdateLock()->unlock();
}

void OwnCloudServiceRoot::addNewCategory() {
}

QMap<int, QVariant> OwnCloudServiceRoot::storeCustomFeedsData() {
	QMap<int, QVariant> custom_data;

	foreach (const Feed* feed, getSubTreeFeeds()) {
		QVariantMap feed_custom_data;
		feed_custom_data.insert(QSL("auto_update_interval"), feed->autoUpdateInitialInterval());
		feed_custom_data.insert(QSL("auto_update_type"), feed->autoUpdateType());
		custom_data.insert(feed->customId(), feed_custom_data);
	}

	return custom_data;
}

void OwnCloudServiceRoot::restoreCustomFeedsData(const QMap<int, QVariant>& data, const QHash<int, Feed*>& feeds) {
	QMapIterator<int, QVariant> i(data);

	while (i.hasNext()) {
		i.next();
		const int custom_id = i.key();

		if (feeds.contains(custom_id)) {
			Feed* feed = feeds.value(custom_id);
			QVariantMap feed_custom_data = i.value().toMap();
			feed->setAutoUpdateInitialInterval(feed_custom_data.value(QSL("auto_update_interval")).toInt());
			feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(feed_custom_data.value(QSL("auto_update_type")).toInt()));
		}
	}
}

RootItem* OwnCloudServiceRoot::obtainNewTreeForSyncIn() const {
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
