// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include "definitions/definitions.h"
#include "services/tt-rss/definitions.h"
#include "network-web/networkfactory.h"


TtRssNetworkFactory::TtRssNetworkFactory()
  : m_url(QString()), m_username(QString()), m_password(QString()), m_session_Id(QString()) {
}

TtRssNetworkFactory::~TtRssNetworkFactory() {
}

QString TtRssNetworkFactory::url() const {
  return m_url;
}

void TtRssNetworkFactory::setUrl(const QString &url) {
  m_url = url;
}

QString TtRssNetworkFactory::username() const {
  return m_username;
}

void TtRssNetworkFactory::setUsername(const QString &username) {
  m_username = username;
}

QString TtRssNetworkFactory::password() const {
  return m_password;
}

void TtRssNetworkFactory::setPassword(const QString &password) {
  m_password = password;
}

LoginResult TtRssNetworkFactory::login() {
  QtJson::JsonObject json;
  json["op"] = "login";
  json["user"] = m_username;
  json["password"] = m_password;

  QByteArray result;
  NetworkResult res = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result);

  if (res.first != QNetworkReply::NoError) {
    return LoginResult(res.first, TtRssLoginResponse());
  }
  else {
    LoginResult result(res.first, TtRssLoginResponse(QString::fromUtf8(result)));
    m_session_Id = result.second.sessionId();
    return result;
  }
}

TtRssResponse::TtRssResponse(const QString &raw_content) {
  m_rawContent = QtJson::parse(raw_content).toMap();
}

TtRssResponse::~TtRssResponse() {
}

bool TtRssResponse::isLoaded() const {
  return !m_rawContent.empty();
}

int TtRssResponse::seq() const {
  if (!isLoaded()) {
    return -1;
  }
  else {
    return m_rawContent["seq"].toInt();
  }
}

int TtRssResponse::status() const {
  if (!isLoaded()) {
    return -1;
  }
  else {
    return m_rawContent["status"].toInt();
  }
}


TtRssLoginResponse::TtRssLoginResponse(const QString &raw_content) : TtRssResponse(raw_content) {
}

TtRssLoginResponse::~TtRssLoginResponse() {
}

int TtRssLoginResponse::apiLevel() const {
  if (!isLoaded()) {
    return -1;
  }
  else {
    return m_rawContent["content"].toMap()["api_level"].toInt();
  }
}

QString TtRssLoginResponse::sessionId() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent["content"].toMap()["session_id"].toString();
  }
}

QString TtRssLoginResponse::error() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent["content"].toMap()["error"].toString();
  }
}

bool TtRssLoginResponse::hasError() const {
  if (!isLoaded()) {
    return false;
  }
  else {
    return m_rawContent["content"].toMap().contains("error");
  }
}
