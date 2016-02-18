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

#include "services/owncloud/network/owncloudnetworkfactory.h"

#include "services/owncloud/definitions.h"
#include "network-web/networkfactory.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QPixmap>


OwnCloudNetworkFactory::OwnCloudNetworkFactory()
  : m_url(QString()), m_forceServerSideUpdate(false),
    m_authUsername(QString()), m_authPassword(QString()), m_urlUser(QString()), m_urlStatus(QString()),
    m_urlFolders(QString()), m_urlFeeds(QString()), m_userId(QString()) {
}

OwnCloudNetworkFactory::~OwnCloudNetworkFactory() {
}

QString OwnCloudNetworkFactory::url() const {
  return m_url;
}

void OwnCloudNetworkFactory::setUrl(const QString &url) {
  m_url = url;
  QString working_url;

  if (url.endsWith('/')) {
    working_url = url;
  }
  else {
    working_url = url + '/';
  }

  // Store endpoints.
  m_urlUser = working_url + API_PATH + "user";
  m_urlStatus = working_url + API_PATH + "status";
  m_urlFolders = working_url + API_PATH + "folders";
  m_urlFeeds = working_url + API_PATH + "feeds";
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

void OwnCloudNetworkFactory::setAuthUsername(const QString &auth_username) {
  m_authUsername = auth_username;
}

QString OwnCloudNetworkFactory::authPassword() const {
  return m_authPassword;
}

void OwnCloudNetworkFactory::setAuthPassword(const QString &auth_password) {
  m_authPassword = auth_password;
}

QNetworkReply::NetworkError OwnCloudNetworkFactory::lastError() const {
  return m_lastError;
}

OwnCloudUserResponse OwnCloudNetworkFactory::userInfo() {
  QByteArray result_raw;
  NetworkResult network_reply = NetworkFactory::downloadFile(m_urlUser,
                                                             qApp->settings()->value(GROUP(Feeds),
                                                                                     SETTING(Feeds::UpdateTimeout)).toInt(),
                                                             result_raw,
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
  NetworkResult network_reply = NetworkFactory::downloadFile(m_urlStatus,
                                                             qApp->settings()->value(GROUP(Feeds),
                                                                                     SETTING(Feeds::UpdateTimeout)).toInt(),
                                                             result_raw,
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
  NetworkResult network_reply = NetworkFactory::downloadFile(m_urlFolders,
                                                             qApp->settings()->value(GROUP(Feeds),
                                                                                     SETTING(Feeds::UpdateTimeout)).toInt(),
                                                             result_raw,
                                                             true, m_authUsername, m_authPassword,
                                                             true);
  if (network_reply.first != QNetworkReply::NoError) {
    qWarning("ownCloud: Obtaining of categories failed with error %d.", network_reply.first);
    m_lastError = network_reply.first;

    return OwnCloudGetFeedsCategoriesResponse();
  }

  QString content_categories = QString::fromUtf8(result_raw);

  // Now, obtain feeds.
  network_reply = NetworkFactory::downloadFile(m_urlFeeds,
                                               qApp->settings()->value(GROUP(Feeds),
                                                                       SETTING(Feeds::UpdateTimeout)).toInt(),
                                               result_raw,
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

QString OwnCloudNetworkFactory::userId() const {
  return m_userId;
}

void OwnCloudNetworkFactory::setUserId(const QString &userId) {
  m_userId = userId;
}

OwnCloudResponse::OwnCloudResponse(const QString &raw_content) {
  m_rawContent = QtJson::parse(raw_content).toMap();
}

OwnCloudResponse::~OwnCloudResponse() {
}

bool OwnCloudResponse::isLoaded() const {
  return !m_rawContent.empty();
}

QString OwnCloudResponse::toString() const {
  return QtJson::serializeStr(m_rawContent);
}


OwnCloudUserResponse::OwnCloudUserResponse(const QString &raw_content) : OwnCloudResponse(raw_content) {
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
    return QDateTime::fromMSecsSinceEpoch(m_rawContent["lastLoginTimestamp"].value<qint64>());
  }
  else {
    return QDateTime();
  }
}

QIcon OwnCloudUserResponse::avatar() const {
  if (isLoaded()) {
    QString image_data = m_rawContent["avatar"].toMap()["data"].toString();
    QByteArray decoded_data = QByteArray::fromBase64(image_data.toLocal8Bit());
    QPixmap image;

    if (image.loadFromData(decoded_data)) {
      return QIcon(image);
    }
  }

  return QIcon();
}


OwnCloudStatusResponse::OwnCloudStatusResponse(const QString &raw_content) : OwnCloudResponse(raw_content) {
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
    return m_rawContent["warnings"].toMap()["improperlyConfiguredCron"].toBool();
  }
  else {
    return false;
  }
}


OwnCloudGetFeedsCategoriesResponse::OwnCloudGetFeedsCategoriesResponse(const QString &raw_categories,
                                                                       const QString &raw_feeds)
  : m_contentCategories(QString()), m_contentFeeds(QString()) {
}

OwnCloudGetFeedsCategoriesResponse::~OwnCloudGetFeedsCategoriesResponse() {
}

RootItem *OwnCloudGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons) const {
  // TODO: TODO
  return NULL;
}
