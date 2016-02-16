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
    m_authUsername(QString()), m_authPassword(QString()), m_urlUser(QString()), m_urlStatus(QString()) {
}

OwnCloudNetworkFactory::~OwnCloudNetworkFactory() {
}

QString OwnCloudNetworkFactory::url() const {
  return m_url;
}

void OwnCloudNetworkFactory::setUrl(const QString &url) {
  if (url.endsWith('/')) {
    m_url = url;
  }
  else {
    m_url = url + '/';
  }

  // Store endpoints.
  m_urlUser = m_url + "index.php/apps/news/api/v1-2/user";
  m_urlStatus = m_url + "index.php/apps/news/api/v1-2/status";
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
