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

#include "services/owncloud/network/owncloudnetworkfactory.h"

#include "services/owncloud/definitions.h"
#include "network-web/networkfactory.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/rootitem.h"
#include "services/owncloud/owncloudcategory.h"
#include "services/owncloud/owncloudfeed.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>


OwnCloudNetworkFactory::OwnCloudNetworkFactory()
	: m_url(QString()), m_fixedUrl(QString()), m_forceServerSideUpdate(false),
	  m_authUsername(QString()), m_authPassword(QString()), m_urlUser(QString()), m_urlStatus(QString()),
	  m_urlFolders(QString()), m_urlFeeds(QString()), m_urlMessages(QString()), m_urlFeedsUpdate(QString()),
	  m_urlDeleteFeed(QString()), m_urlRenameFeed(QString()), m_userId(QString()) {
}

OwnCloudNetworkFactory::~OwnCloudNetworkFactory() {
}

QString OwnCloudNetworkFactory::url() const {
	return m_url;
}

void OwnCloudNetworkFactory::setUrl(const QString& url) {
	m_url = url;

	if (url.endsWith('/')) {
		m_fixedUrl = url;
	}

	else {
		m_fixedUrl = url + '/';
	}

	// Store endpoints.
	m_urlUser = m_fixedUrl + API_PATH + "user";
	m_urlStatus = m_fixedUrl + API_PATH + "status";
	m_urlFolders = m_fixedUrl + API_PATH + "folders";
	m_urlFeeds = m_fixedUrl + API_PATH + "feeds";
	m_urlMessages = m_fixedUrl + API_PATH + "items?id=%1&batchSize=%2&type=%3";
	m_urlFeedsUpdate = m_fixedUrl + API_PATH + "feeds/update?userId=%1&feedId=%2";
	m_urlDeleteFeed = m_fixedUrl + API_PATH + "feeds/%1";
	m_urlRenameFeed = m_fixedUrl + API_PATH + "feeds/%1/rename";
	setUserId(QString());
}

bool OwnCloudNetworkFactory::forceServerSideUpdate() const {
	return m_forceServerSideUpdate;
}

void OwnCloudNetworkFactory::setForceServerSideUpdate(bool force_update) {
	m_forceServerSideUpdate = force_update;
}

QString OwnCloudNetworkFactory::authUsername() const {
	return m_authUsername;
}

void OwnCloudNetworkFactory::setAuthUsername(const QString& auth_username) {
	m_authUsername = auth_username;
	setUserId(QString());
}

QString OwnCloudNetworkFactory::authPassword() const {
	return m_authPassword;
}

void OwnCloudNetworkFactory::setAuthPassword(const QString& auth_password) {
	m_authPassword = auth_password;
	setUserId(QString());
}

QNetworkReply::NetworkError OwnCloudNetworkFactory::lastError() const {
	return m_lastError;
}

OwnCloudUserResponse OwnCloudNetworkFactory::userInfo() {
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlUser,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QByteArray(), QString(), result_raw,
	                              QNetworkAccessManager::GetOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);
	OwnCloudUserResponse user_response(QString::fromUtf8(result_raw));

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Obtaining user info failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return user_response;
}

OwnCloudStatusResponse OwnCloudNetworkFactory::status() {
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlStatus,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QByteArray(), QString(), result_raw,
	                              QNetworkAccessManager::GetOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);
	OwnCloudStatusResponse status_response(QString::fromUtf8(result_raw));

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Obtaining status info failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return status_response;
}

OwnCloudGetFeedsCategoriesResponse OwnCloudNetworkFactory::feedsCategories() {
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlFolders,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QByteArray(), QString(), result_raw,
	                              QNetworkAccessManager::GetOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Obtaining of categories failed with error %d.", network_reply.first);
		m_lastError = network_reply.first;
		return OwnCloudGetFeedsCategoriesResponse();
	}

	QString content_categories = QString::fromUtf8(result_raw);
	// Now, obtain feeds.
	network_reply = NetworkFactory::performNetworkOperation(m_urlFeeds,
	                                                        qApp->settings()->value(GROUP(Feeds),
	                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
	                                                        QByteArray(), QString(), result_raw,
	                                                        QNetworkAccessManager::GetOperation,
	                                                        true, m_authUsername, m_authPassword,
	                                                        true);

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Obtaining of feeds failed with error %d.", network_reply.first);
		m_lastError = network_reply.first;
		return OwnCloudGetFeedsCategoriesResponse();
	}

	QString content_feeds = QString::fromUtf8(result_raw);
	m_lastError = network_reply.first;
	return OwnCloudGetFeedsCategoriesResponse(content_categories, content_feeds);
}

bool OwnCloudNetworkFactory::deleteFeed(int feed_id) {
	QString final_url = m_urlDeleteFeed.arg(QString::number(feed_id));
	QByteArray raw_output;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QByteArray(), QString(),
	                              raw_output, QNetworkAccessManager::DeleteOperation,
	                              true, m_authUsername, m_authPassword, true);
	m_lastError = network_reply.first;

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Obtaining of categories failed with error %d.", network_reply.first);
		return false;
	}

	else {
		return true;
	}
}

bool OwnCloudNetworkFactory::createFeed(const QString& url, int parent_id) {
	QJsonObject json;
	json["url"] = url;
	json["folderId"] = parent_id;
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlFeeds,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              QSL("application/json"),
	                              result_raw,
	                              QNetworkAccessManager::PostOperation,
	                              true, m_authUsername, m_authPassword, true);
	m_lastError = network_reply.first;

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Creating of category failed with error %d.", network_reply.first);
		return false;
	}

	else {
		return true;
	}
}

bool OwnCloudNetworkFactory::renameFeed(const QString& new_name, int feed_id) {
	QString final_url = m_urlRenameFeed.arg(QString::number(feed_id));
	QByteArray result_raw;
	QJsonObject json;
	json["feedTitle"] = new_name;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              QSL("application/json"), result_raw,
	                              QNetworkAccessManager::PutOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);
	m_lastError = network_reply.first;

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Renaming of feed failed with error %d.", network_reply.first);
		return false;
	}

	else {
		return true;
	}
}

OwnCloudGetMessagesResponse OwnCloudNetworkFactory::getMessages(int feed_id) {
	if (forceServerSideUpdate()) {
		triggerFeedUpdate(feed_id);
	}

	QString final_url = m_urlMessages.arg(QString::number(feed_id),
	                                      QString::number(-1),
	                                      QString::number(0));
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QByteArray(), QString(), result_raw,
	                              QNetworkAccessManager::GetOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);
	OwnCloudGetMessagesResponse msgs_response(QString::fromUtf8(result_raw));

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Obtaining messages failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return msgs_response;
}

QNetworkReply::NetworkError OwnCloudNetworkFactory::triggerFeedUpdate(int feed_id) {
	if (userId().isEmpty()) {
		// We need to get user ID first.
		OwnCloudUserResponse info = userInfo();

		if (lastError() != QNetworkReply::NoError) {
			return lastError();
		}

		else {
			// We have new user ID, set it up.
			setUserId(info.userId());
		}
	}

	// Now, we can trigger the update.
	QByteArray raw_output;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlFeedsUpdate.arg(userId(),
	                              QString::number(feed_id)),
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QByteArray(), QString(), raw_output,
	                              QNetworkAccessManager::GetOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Feeds update failed with error %d.", network_reply.first);
	}

	return (m_lastError = network_reply.first);
}

QNetworkReply::NetworkError OwnCloudNetworkFactory::markMessagesRead(RootItem::ReadStatus status,
        const QStringList& custom_ids) {
	QJsonObject json;
	QJsonArray ids;
	QByteArray raw_output;
	QString final_url;

	if (status == RootItem::Read) {
		final_url = m_fixedUrl + API_PATH + "items/read/multiple";
	}

	else {
		final_url = m_fixedUrl + API_PATH + "items/unread/multiple";
	}

	foreach (const QString& id, custom_ids) {
		ids.append(QJsonValue(id.toInt()));
	}

	json["items"] = ids;
	qDebug() << QSL("Raw output for marking msgs read with Nextcloud is : \n\n") << QString::fromUtf8(raw_output);
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              QSL("application/json"),
	                              raw_output,
	                              QNetworkAccessManager::PutOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Marking messages as (un)read failed with error %d.", network_reply.first);
	}

	return (m_lastError = network_reply.first);
}

QNetworkReply::NetworkError OwnCloudNetworkFactory::markMessagesStarred(RootItem::Importance importance,
        const QStringList& feed_ids,
        const QStringList& guid_hashes) {
	QJsonObject json;
	QJsonArray  ids;
	QByteArray raw_output;
	QString final_url;

	if (importance == RootItem::Important) {
		final_url = m_fixedUrl + API_PATH + "items/star/multiple";
	}

	else {
		final_url = m_fixedUrl + API_PATH + "items/unstar/multiple";
	}

	for (int i = 0; i < feed_ids.size(); i++) {
		QJsonObject item;
		item["feedId"] = feed_ids.at(i);
		item["guidHash"] = guid_hashes.at(i);
		ids.append(item);
	}

	json["items"] = ids;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
	                              qApp->settings()->value(GROUP(Feeds),
	                                                      SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              "application/json",
	                              raw_output,
	                              QNetworkAccessManager::PutOperation,
	                              true, m_authUsername, m_authPassword,
	                              true);

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("ownCloud: Marking messages as (un)starred failed with error %d.", network_reply.first);
	}

	return (m_lastError = network_reply.first);
}

QString OwnCloudNetworkFactory::userId() const {
	return m_userId;
}

void OwnCloudNetworkFactory::setUserId(const QString& userId) {
	m_userId = userId;
}

OwnCloudResponse::OwnCloudResponse(const QString& raw_content) {
	m_rawContent = QJsonDocument::fromJson(raw_content.toUtf8()).object();
	m_emptyString = raw_content.isEmpty();
}

OwnCloudResponse::~OwnCloudResponse() {
}

bool OwnCloudResponse::isLoaded() const {
	return !m_emptyString && !m_rawContent.isEmpty();
}

QString OwnCloudResponse::toString() const {
	return QJsonDocument(m_rawContent).toJson(QJsonDocument::Compact);
}

OwnCloudUserResponse::OwnCloudUserResponse(const QString& raw_content) : OwnCloudResponse(raw_content) {
}

OwnCloudUserResponse::~OwnCloudUserResponse() {
}

QString OwnCloudUserResponse::displayName() const {
	if (isLoaded()) {
		return m_rawContent["displayName"].toString();
	}

	else {
		return QString();
	}
}

QString OwnCloudUserResponse::userId() const {
	if (isLoaded()) {
		return m_rawContent["userId"].toString();
	}

	else {
		return QString();
	}
}

QDateTime OwnCloudUserResponse::lastLoginTime() const {
	if (isLoaded()) {
		return QDateTime::fromMSecsSinceEpoch(m_rawContent["lastLoginTimestamp"].toDouble());
	}

	else {
		return QDateTime();
	}
}

QIcon OwnCloudUserResponse::avatar() const {
	if (isLoaded()) {
		QString image_data = m_rawContent["avatar"].toObject()["data"].toString();
		QByteArray decoded_data = QByteArray::fromBase64(image_data.toLocal8Bit());
		QPixmap image;

		if (image.loadFromData(decoded_data)) {
			return QIcon(image);
		}
	}

	return QIcon();
}


OwnCloudStatusResponse::OwnCloudStatusResponse(const QString& raw_content) : OwnCloudResponse(raw_content) {
}

OwnCloudStatusResponse::~OwnCloudStatusResponse() {
}

QString OwnCloudStatusResponse::version() const {
	if (isLoaded()) {
		return m_rawContent["version"].toString();
	}

	else {
		return QString();
	}
}

bool OwnCloudStatusResponse::misconfiguredCron() const {
	if (isLoaded()) {
		return m_rawContent["warnings"].toObject()["improperlyConfiguredCron"].toBool();
	}

	else {
		return false;
	}
}


OwnCloudGetFeedsCategoriesResponse::OwnCloudGetFeedsCategoriesResponse(const QString& raw_categories,
        const QString& raw_feeds)
	: m_contentCategories(raw_categories), m_contentFeeds(raw_feeds) {
}

OwnCloudGetFeedsCategoriesResponse::~OwnCloudGetFeedsCategoriesResponse() {
}

RootItem* OwnCloudGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons) const {
	RootItem* parent = new RootItem();
	QMap<int, RootItem*> cats;
	cats.insert(0, parent);

	// Process categories first, then process feeds.
	foreach (const QJsonValue& cat, QJsonDocument::fromJson(m_contentCategories.toUtf8()).object()["folders"].toArray()) {
		QJsonObject item = cat.toObject();
		OwnCloudCategory* category = new OwnCloudCategory();
		category->setTitle(item["name"].toString());
		category->setCustomId(item["id"].toInt());
		cats.insert(category->customId(), category);
		// All categories in ownCloud are top-level.
		parent->appendChild(category);
	}

	// We have categories added, now add all feeds.
	foreach (const QJsonValue& fed, QJsonDocument::fromJson(m_contentFeeds.toUtf8()).object()["feeds"].toArray()) {
		QJsonObject item = fed.toObject();
		OwnCloudFeed* feed = new OwnCloudFeed();

		if (obtain_icons) {
			QString icon_path = item["faviconLink"].toString();

			if (!icon_path.isEmpty()) {
				QByteArray icon_data;

				if (NetworkFactory::performNetworkOperation(icon_path, DOWNLOAD_TIMEOUT,
				                                            QByteArray(), QString(), icon_data,
				                                            QNetworkAccessManager::GetOperation).first == QNetworkReply::NoError) {
					// Icon downloaded, set it up.
					QPixmap icon_pixmap;
					icon_pixmap.loadFromData(icon_data);
					feed->setIcon(QIcon(icon_pixmap));
				}
			}
		}

		feed->setUrl(item["link"].toString());
		feed->setTitle(item["title"].toString());
		feed->setCustomId(item["id"].toInt());
		qDebug("Custom ID of next fetched Nextcloud feed is '%d'.", item["id"].toInt());
		cats.value(item["folderId"].toInt())->appendChild(feed);
	}

	return parent;
}


OwnCloudGetMessagesResponse::OwnCloudGetMessagesResponse(const QString& raw_content) : OwnCloudResponse(raw_content) {
}

OwnCloudGetMessagesResponse::~OwnCloudGetMessagesResponse() {
}

QList<Message> OwnCloudGetMessagesResponse::messages() const {
	QList<Message> msgs;

	foreach (const QJsonValue& message, m_rawContent["items"].toArray()) {
		QJsonObject message_map = message.toObject();
		Message msg;
		msg.m_author = message_map["author"].toString();
		msg.m_contents = message_map["body"].toString();
		msg.m_created = TextFactory::parseDateTime(message_map["pubDate"].toDouble() * 1000);
		msg.m_createdFromFeed = true;
		msg.m_customId = QString::number(message_map["id"].toInt());
		msg.m_customHash = message_map["guidHash"].toString();
		QString enclosure_link = message_map["enclosureLink"].toString();

		if (!enclosure_link.isEmpty()) {
			Enclosure enclosure;
			enclosure.m_mimeType = message_map["enclosureMime"].toString();
			enclosure.m_url = enclosure_link;
			msg.m_enclosures.append(enclosure);
		}

		msg.m_feedId = QString::number(message_map["feedId"].toInt());
		msg.m_isImportant = message_map["starred"].toBool();
		msg.m_isRead = !message_map["unread"].toBool();
		msg.m_title = message_map["title"].toString();
		msg.m_url = message_map["url"].toString();
		msgs.append(msg);
	}

	return msgs;
}
