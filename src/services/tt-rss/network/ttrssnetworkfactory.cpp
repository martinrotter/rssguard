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
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrsscategory.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"

#include <QPair>


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


TtRssLoginResponse TtRssNetworkFactory::login(QNetworkReply::NetworkError &error) {
  if (!m_sessionId.isEmpty()) {
    logout(error);
  }

  QtJson::JsonObject json;
  json["op"] = "login";
  json["user"] = m_username;
  json["password"] = m_password;

  QByteArray result_raw;
  NetworkResult network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);
  TtRssLoginResponse login_response(QString::fromUtf8(result_raw));

  if (network_reply.first == QNetworkReply::NoError) {
    m_sessionId = login_response.sessionId();
  }

  error = network_reply.first;
  return login_response;
}

TtRssResponse TtRssNetworkFactory::logout(QNetworkReply::NetworkError &error) {
  if (!m_sessionId.isEmpty()) {

    QtJson::JsonObject json;
    json["op"] = "logout";
    json["sid"] = m_sessionId;

    QByteArray result_raw;
    NetworkResult network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);

    error = network_reply.first;
    return TtRssResponse(QString::fromUtf8(result_raw));
  }
  else {
    error = QNetworkReply::NoError;
    return TtRssResponse();
  }
}

TtRssGetFeedsCategoriesResponse TtRssNetworkFactory::getFeedsCategories(QNetworkReply::NetworkError &error) {
  QtJson::JsonObject json;
  json["op"] = "getFeedTree";
  json["sid"] = m_sessionId;
  json["include_empty"] = true;

  QByteArray result_raw;
  NetworkResult network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);
  TtRssGetFeedsCategoriesResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(error);
    json["sid"] = m_sessionId;

    network_reply = NetworkFactory::uploadData(m_url, DOWNLOAD_TIMEOUT, QtJson::serialize(json), CONTENT_TYPE, result_raw);
    result = TtRssGetFeedsCategoriesResponse(QString::fromUtf8(result_raw));
  }

  error = network_reply.first;
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


TtRssGetFeedsCategoriesResponse::TtRssGetFeedsCategoriesResponse(const QString &raw_content) : TtRssResponse(raw_content) {

}

TtRssGetFeedsCategoriesResponse::~TtRssGetFeedsCategoriesResponse() {
}

RootItem *TtRssGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons, QString base_address) {
  RootItem *parent = new RootItem();

  // Chop the "api/" from the end of the address.
  base_address.chop(4);

  if (status() == API_STATUS_OK) {
    // We have data, construct object tree according to data.
    QList<QVariant> items_to_process = m_rawContent["content"].toMap()["categories"].toMap()["items"].toList();
    QList<QPair<RootItem*,QVariant> > pairs;

    foreach (QVariant item, items_to_process) {
      pairs.append(QPair<RootItem*,QVariant>(parent, item));
    }

    while (!pairs.isEmpty()) {
      QPair<RootItem*,QVariant> pair = pairs.takeFirst();
      RootItem *act_parent = pair.first;
      QMap<QString,QVariant> item = pair.second.toMap();

      int item_id = item["bare_id"].toInt();
      bool is_category = item.contains("type") && item["type"].toString() == GFT_TYPE_CATEGORY;

      if (item_id >= 0) {
        if (is_category) {
          if (item_id == 0) {
            // This is "Uncategorized" category, all its feeds belong to top-level root.
            if (item.contains("items")) {
              foreach (QVariant child_feed, item["items"].toList()) {
                pairs.append(QPair<RootItem*,QVariant>(parent, child_feed));
              }
            }
          }
          else {
            TtRssCategory *category = new TtRssCategory();

            category->setTitle(item["name"].toString());
            category->setCustomId(item_id);
            act_parent->appendChild(category);

            if (item.contains("items")) {
              foreach (QVariant child, item["items"].toList()) {
                pairs.append(QPair<RootItem*,QVariant>(category, child));
              }
            }
          }
        }
        else {
          // We have feed.
          TtRssFeed *feed = new TtRssFeed();

          if (obtain_icons) {
            QString icon_path = item["icon"].type() == QVariant::String ? item["icon"].toString() : QString();

            if (!icon_path.isEmpty()) {
              // Chop the "api/" suffix out and append
              QString full_icon_address = base_address + QL1C('/') + icon_path;
              QByteArray icon_data;

              if (NetworkFactory::downloadFile(full_icon_address, DOWNLOAD_TIMEOUT, icon_data).first == QNetworkReply::NoError) {
                // Icon downloaded, set it up.
                QPixmap icon_pixmap;
                icon_pixmap.loadFromData(icon_data);
                feed->setIcon(QIcon(icon_pixmap));
              }
            }
          }

          // TODO: stahnout a nastavit ikonu
          feed->setTitle(item["name"].toString());
          feed->setCustomId(item_id);
          act_parent->appendChild(feed);
        }
      }
    }
  }

  return parent;
}
