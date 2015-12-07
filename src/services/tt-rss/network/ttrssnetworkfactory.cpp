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
#include "core/rootitem.h"
#include "services/tt-rss/definitions.h"
#include "network-web/networkfactory.h"


TtRssNetworkFactory::TtRssNetworkFactory()
  : m_url(QString()), m_username(QString()), m_password(QString()), m_sessionId(QString()) {
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

// TODO: ukazky

/* ukazky
 * prihlaseni - curl -L -d '{"op":"login","user":"admin","password":"XXX"}' http://rss.rotterovi.eu/api/
 * ziska seznam VSECH zprav - curl -L -d '{"sid":"xxx","op":"getHeadlines","feed_id":-4,"include_nested":true,"include_attachments":true,"show_content":true}' http://rss.rotterovi.eu/api/
 * seznam kategorii vcetne unread countu - curl -L -d '{"sid":"e9528741496d0d6aa5021e67ca519823","op":"getCategories","include_nested":true,"include_empty":false}' http://rss.rotterovi.eu/api/
 * */


LoginResult TtRssNetworkFactory::login() {
  if (!m_sessionId.isEmpty()) {
    logout();
  }

  QtJson::JsonObject json;
  json["op"] = "login";
  json["user"] = m_username;
  json["password"] = m_password;

  QByteArray result_raw;
  NetworkResult network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);
  LoginResult result(network_reply.first, TtRssLoginResponse(QString::fromUtf8(result_raw)));

  if (network_reply.first == QNetworkReply::NoError) {
    m_sessionId = result.second.sessionId();
  }

  return result;
}

LogoutResult TtRssNetworkFactory::logout() {
  QtJson::JsonObject json;
  json["op"] = "logout";
  json["sid"] = m_sessionId;

  QByteArray result_raw;
  NetworkResult network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);

  return LogoutResult(network_reply.first, TtRssResponse(QString::fromUtf8(result_raw)));
}

GetFeedTreeResult TtRssNetworkFactory::getFeedTree() {
  QtJson::JsonObject json;
  json["op"] = "getFeedTree";
  json["sid"] = m_sessionId;
  json["include_empty"] = true;

  QByteArray result_raw;
  NetworkResult network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);
  GetFeedTreeResult result(network_reply.first, TtRssGetFeedTreeResponse(QString::fromUtf8(result_raw)));

  if (result.second.isNotLoggedIn()) {
    // We are not logged in.
    login();

    network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);
    result = GetFeedTreeResult(network_reply.first, TtRssGetFeedTreeResponse(QString::fromUtf8(result_raw)));
  }

  return result;
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

bool TtRssResponse::isNotLoggedIn() const {
  return status() == API_STATUS_ERR && hasError() && error() == NOT_LOGGED_IN;
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

QString TtRssResponse::error() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent["content"].toMap()["error"].toString();
  }
}

bool TtRssResponse::hasError() const {
  if (!isLoaded()) {
    return false;
  }
  else {
    return m_rawContent["content"].toMap().contains("error");
  }
}


TtRssGetFeedTreeResponse::TtRssGetFeedTreeResponse(const QString &raw_content) : TtRssResponse(raw_content) {

}

TtRssGetFeedTreeResponse::~TtRssGetFeedTreeResponse() {
}

QList<RootItem*> TtRssGetFeedTreeResponse::getTree() {
  QList<RootItem*> items;

  if (status() == API_STATUS_OK) {
    // We have data, construct object tree according to data.
    QList<QVariant> items_to_process = m_rawContent["content"].toMap()["categories"].toMap()["items"].toList();

    processSubtree(true, items, NULL, items_to_process);
  }

  return items;
}

void TtRssGetFeedTreeResponse::processSubtree(bool is_top_level, QList<RootItem *> &top_level_items,
                                              RootItem *parent, const QList<QVariant> &items) {
  foreach (QVariant item, items) {
    QMap<QString,QVariant> map_item = item.toMap();

    if (map_item.contains("type") && map_item["type"].toString() == GFT_TYPE_CATEGORY) {
      // TODO: pokračovat tady

      // We have category, create it, add it to "parent".
      // Then process all its children.
      //
      // TtRssCategory *new_category = new TtRssCategory();
      // naplnit informace.....
      // parent->appendChild(new_category);
      // if (is_top_level) {
      //   top_level_items.append(new_category);
      // }
      // else {
      //   parent->appendChild(new_category);
      // }
      // processSubtree(false, top_level_items, new_category, map_item["items"].toList());
    }
    else {
      // We have feed, add it.
      // TtRssFeed *new_feed = new TtRssFeed();
      // naplnit informace.....
      // parent->appendChild(new_feed);
      // if (is_top_level) {
      //   top_level_items.append(new_feed);
      // }
    }
  }
}
