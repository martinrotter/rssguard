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

#include "services/inoreader/network/inoreadernetworkfactory.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/inoreaderfeed.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

InoreaderNetworkFactory::InoreaderNetworkFactory(QObject* parent) : QObject(parent),
  m_username(QString()), m_batchSize(INOREADER_DEFAULT_BATCH_SIZE),
  m_oauth2(new OAuth2Service(INOREADER_OAUTH_AUTH_URL, INOREADER_OAUTH_TOKEN_URL,
                             INOREADER_OAUTH_CLI_ID, INOREADER_OAUTH_CLI_KEY, INOREADER_OAUTH_SCOPE)) {
  initializeOauth();
}

OAuth2Service* InoreaderNetworkFactory::oauth() const {
  return m_oauth2;
}

QString InoreaderNetworkFactory::userName() const {
  return m_username;
}

int InoreaderNetworkFactory::batchSize() const {
  return m_batchSize;
}

void InoreaderNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

void InoreaderNetworkFactory::initializeOauth() {
  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, [](QString error, QString error_description) {
    Q_UNUSED(error)

    qApp->showGuiMessage("Authentication error - Inoreader", error_description, QSystemTrayIcon::Critical);
  });
}

void InoreaderNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

// NOTE: oauth: https://developers.google.com/oauthplayground/#step3&scopes=read%20write&auth_code=497815bc3362aba9ad60c5ae3e01811fe2da4bb5&refresh_token=bacb9c36f82ba92667282d6175bb857a091e7f0c&access_token_field=094f92bc7aedbd27fbebc3efc9172b258be8944a&url=https%3A%2F%2Fwww.inoreader.com%2Freader%2Fapi%2F0%2Fsubscription%2Flist&content_type=application%2Fjson&http_method=GET&useDefaultOauthCred=unchecked&oauthEndpointSelect=Custom&oauthAuthEndpointValue=https%3A%2F%2Fwww.inoreader.com%2Foauth2%2Fauth%3Fstate%3Dtest&oauthTokenEndpointValue=https%3A%2F%2Fwww.inoreader.com%2Foauth2%2Ftoken&oauthClientId=1000000595&expires_in=3599&oauthClientSecret=_6pYUZgtNLWwSaB9pC1YOz6p4zwu3haL&access_token_issue_date=1506198338&for_access_token=094f92bc7aedbd27fbebc3efc9172b258be8944a&includeCredentials=checked&accessTokenType=bearer&autoRefreshToken=unchecked&accessType=offline&prompt=consent&response_type=code

RootItem* InoreaderNetworkFactory::feedsCategories(bool obtain_icons) {
  Downloader downloader;
  QEventLoop loop;

  downloader.appendRawHeader(QString("Authorization").toLocal8Bit(), m_oauth2->bearer().toLocal8Bit());

  // We need to quit event loop when the download finishes.
  connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);
  downloader.manipulateData(INOREADER_API_LIST_LABELS, QNetworkAccessManager::Operation::GetOperation);
  loop.exec();

  if (downloader.lastOutputError() != QNetworkReply::NetworkError::NoError) {
    return nullptr;
  }

  QString category_data = downloader.lastOutputData();

  downloader.manipulateData(INOREADER_API_LIST_FEEDS, QNetworkAccessManager::Operation::GetOperation);
  loop.exec();

  if (downloader.lastOutputError() != QNetworkReply::NetworkError::NoError) {
    return nullptr;
  }

  QString feed_data = downloader.lastOutputData();

  return decodeFeedCategoriesData(category_data, feed_data, obtain_icons);
}

QList<Message> InoreaderNetworkFactory::messages(const QString& stream_id, bool* is_error) {
  Downloader downloader;
  QEventLoop loop;
  QString target_url = INOREADER_API_FEED_CONTENTS;

  target_url += QSL("/") + QUrl::toPercentEncoding(stream_id) + QString("?n=%1").arg(batchSize());
  downloader.appendRawHeader(QString("Authorization").toLocal8Bit(), m_oauth2->bearer().toLocal8Bit());

  IOFactory::writeTextFile("aa.bb", target_url.toUtf8());

  // We need to quit event loop when the download finishes.
  connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);
  downloader.manipulateData(target_url, QNetworkAccessManager::Operation::GetOperation);
  loop.exec();

  if (downloader.lastOutputError() != QNetworkReply::NetworkError::NoError) {
    *is_error = true;
    return QList<Message>();
  }
  else {
    QString messages_data = downloader.lastOutputData();

    return decodeMessages(messages_data, stream_id);
  }
}

QList<Message> InoreaderNetworkFactory::decodeMessages(const QString& messages_json_data, const QString& stream_id) {
  QList<Message> messages;
  QJsonArray json = QJsonDocument::fromJson(messages_json_data.toUtf8()).object()["items"].toArray();

  IOFactory::writeTextFile("aa.aa", messages_json_data.toUtf8());

  messages.reserve(json.count());

  foreach (const QJsonValue& obj, json) {
    auto message_obj = obj.toObject();
    Message message;

    message.m_title = message_obj["title"].toString();
    message.m_author = message_obj["author"].toString();
    message.m_created = QDateTime::fromMSecsSinceEpoch(message_obj["published"].toInt());
    message.m_createdFromFeed = true;
    message.m_customId = message_obj["id"].toString();

    auto alternates = message_obj["alternate"].toArray();
    auto enclosures = message_obj["enclosure"].toArray();
    auto categories = message_obj["categories"].toArray();

    foreach (const QJsonValue& alt, alternates) {
      auto alt_obj = alt.toObject();
      QString mime = alt_obj["type"].toString();
      QString href = alt_obj["href"].toString();

      if (mime == QL1S("text/html")) {
        message.m_url = href;
      }
      else {
        message.m_enclosures.append(Enclosure(href, mime));
      }
    }

    foreach (const QJsonValue& enc, enclosures) {
      auto enc_obj = enc.toObject();
      QString mime = enc_obj["type"].toString();
      QString href = enc_obj["href"].toString();

      message.m_enclosures.append(Enclosure(href, mime));
    }

    foreach (const QJsonValue& cat, categories) {
      QString category = cat.toString();

      if (category.contains(INOREADER_STATE_READ)) {
        message.m_isRead = !category.contains(INOREADER_STATE_READING_LIST);
      }
      else if (category.contains(INOREADER_STATE_IMPORTANT)) {
        message.m_isImportant = category.contains(INOREADER_STATE_IMPORTANT);
      }
    }

    message.m_contents = message_obj["summary"].toObject()["content"].toString();
    message.m_feedId = stream_id;

    messages.append(message);
  }

  return messages;
}

RootItem* InoreaderNetworkFactory::decodeFeedCategoriesData(const QString& categories, const QString& feeds, bool obtain_icons) {
  RootItem* parent = new RootItem();
  QJsonArray json = QJsonDocument::fromJson(categories.toUtf8()).object()["tags"].toArray();

  QMap<QString, RootItem*> cats;
  cats.insert(QString(), parent);

  foreach (const QJsonValue& obj, json) {
    auto label = obj.toObject();
    QString label_id = label["id"].toString();

    if (label_id.contains(QSL("/label/"))) {
      // We have label (not "state").
      Category* category = new Category();

      category->setDescription(label["htmlUrl"].toString());
      category->setTitle(label_id.mid(label_id.lastIndexOf(QL1C('/')) + 1));
      category->setCustomId(label_id);

      cats.insert(category->customId(), category);

      // All categories in ownCloud are top-level.
      parent->appendChild(category);
    }
  }

  json = QJsonDocument::fromJson(feeds.toUtf8()).object()["subscriptions"].toArray();

  foreach (const QJsonValue& obj, json) {
    auto subscription = obj.toObject();
    QString id = subscription["id"].toString();
    QString title = subscription["title"].toString();
    QString url = subscription["htmlUrl"].toString();
    QString parent_label;
    QJsonArray categories = subscription["categories"].toArray();

    foreach (const QJsonValue& cat, categories) {
      QString potential_id = cat.toObject()["id"].toString();

      if (potential_id.contains(QSL("/label/"))) {
        parent_label = potential_id;
        break;
      }
    }

    // We have label (not "state").
    InoreaderFeed* feed = new InoreaderFeed();

    feed->setDescription(url);
    feed->setUrl(url);
    feed->setTitle(title);
    feed->setCustomId(id);

    if (obtain_icons) {
      QString icon_url = subscription["iconUrl"].toString();

      if (!icon_url.isEmpty()) {
        QByteArray icon_data;

        if (NetworkFactory::performNetworkOperation(icon_url, DOWNLOAD_TIMEOUT,
                                                    QByteArray(), QString(), icon_data,
                                                    QNetworkAccessManager::GetOperation).first == QNetworkReply::NoError) {
          // Icon downloaded, set it up.
          QPixmap icon_pixmap;

          icon_pixmap.loadFromData(icon_data);
          feed->setIcon(QIcon(icon_pixmap));
        }
      }
    }

    if (cats.contains(parent_label)) {
      cats[parent_label]->appendChild(feed);
    }
  }

  return parent;
}
