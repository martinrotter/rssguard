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

#include "services/gmail/network/gmailnetworkfactory.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailfeed.h"
#include "services/gmail/gmailserviceroot.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

GmailNetworkFactory::GmailNetworkFactory(QObject* parent) : QObject(parent),
  m_service(nullptr), m_username(QString()), m_batchSize(GMAIL_DEFAULT_BATCH_SIZE),
  m_oauth2(new OAuth2Service(GMAIL_OAUTH_AUTH_URL, GMAIL_OAUTH_TOKEN_URL,
                             QString(), QString(), GMAIL_OAUTH_SCOPE)) {
  initializeOauth();
}

void GmailNetworkFactory::setService(GmailServiceRoot* service) {
  m_service = service;
}

OAuth2Service* GmailNetworkFactory::oauth() const {
  return m_oauth2;
}

QString GmailNetworkFactory::userName() const {
  return m_username;
}

int GmailNetworkFactory::batchSize() const {
  return m_batchSize;
}

void GmailNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

void GmailNetworkFactory::initializeOauth() {
  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, this, &GmailNetworkFactory::onTokensError);
  connect(m_oauth2, &OAuth2Service::authFailed, this, &GmailNetworkFactory::onAuthFailed);
  connect(m_oauth2, &OAuth2Service::tokensReceived, [this](QString access_token, QString refresh_token, int expires_in) {
    Q_UNUSED(expires_in)

    if (m_service != nullptr && !access_token.isEmpty() && !refresh_token.isEmpty()) {
      QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
      DatabaseQueries::storeNewInoreaderTokens(database, refresh_token, m_service->accountId());

      qApp->showGuiMessage(tr("Logged in successfully"),
                           tr("Your login to Inoreader was authorized."),
                           QSystemTrayIcon::MessageIcon::Information);
    }
  });
}

void GmailNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

RootItem* GmailNetworkFactory::feedsCategories(bool obtain_icons) {
  Downloader downloader;
  QEventLoop loop;
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return nullptr;
  }

  downloader.appendRawHeader(QString("Authorization").toLocal8Bit(), bearer.toLocal8Bit());

  // We need to quit event loop when the download finishes.
  connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);

  // TODO: dodělat
  //downloader.manipulateData(INOREADER_API_LIST_LABELS, QNetworkAccessManager::Operation::GetOperation);
  loop.exec();

  if (downloader.lastOutputError() != QNetworkReply::NetworkError::NoError) {
    return nullptr;
  }

  QString category_data = downloader.lastOutputData();

  // TODO: dodělat
  //downloader.manipulateData(INOREADER_API_LIST_FEEDS, QNetworkAccessManager::Operation::GetOperation);
  loop.exec();

  if (downloader.lastOutputError() != QNetworkReply::NetworkError::NoError) {
    return nullptr;
  }

  QString feed_data = downloader.lastOutputData();

  return decodeFeedCategoriesData(category_data, feed_data, obtain_icons);
}

QList<Message> GmailNetworkFactory::messages(const QString& stream_id, Feed::Status& error) {
  Downloader downloader;
  QEventLoop loop;
  QString target_url;// TODO: dodělat
  // = INOREADER_API_FEED_CONTENTS;
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    error = Feed::Status::AuthError;
    return QList<Message>();
  }

  target_url += QSL("/") + QUrl::toPercentEncoding(stream_id) + QString("?n=%1").arg(batchSize());
  downloader.appendRawHeader(QString("Authorization").toLocal8Bit(), bearer.toLocal8Bit());

  // We need to quit event loop when the download finishes.
  connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);
  downloader.manipulateData(target_url, QNetworkAccessManager::Operation::GetOperation);
  loop.exec();

  if (downloader.lastOutputError() != QNetworkReply::NetworkError::NoError) {
    error = Feed::Status::NetworkError;
    return QList<Message>();
  }
  else {
    QString messages_data = downloader.lastOutputData();

    error = Feed::Status::Normal;
    return decodeMessages(messages_data, stream_id);
  }
}

void GmailNetworkFactory::markMessagesRead(RootItem::ReadStatus status, const QStringList& custom_ids, bool async) {
  QString target_url;// TODO: dodělat
  // = INOREADER_API_EDIT_TAG;
  // TODO: dodělat
  //

  /*
     if (status == RootItem::ReadStatus::Read) {
     target_url += QString("?a=user/-/") + INOREADER_STATE_READ + "&";
     }
     else {
     target_url += QString("?r=user/-/") + INOREADER_STATE_READ + "&";
     }*/
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return;
  }

  QList<QPair<QByteArray, QByteArray>> headers;
  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));

  QStringList trimmed_ids;
  QRegularExpression regex_short_id(QSL("[0-9a-zA-Z]+$"));

  foreach (const QString& id, custom_ids) {
    QString simplified_id = regex_short_id.match(id).captured();

    trimmed_ids.append(QString("i=") + simplified_id);
  }

  QStringList working_subset;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  working_subset.reserve(trimmed_ids.size() > 200 ? 200 : trimmed_ids.size());

  // Now, we perform messages update in batches (max 200 messages per batch).
  while (!trimmed_ids.isEmpty()) {
    // We take 200 IDs.
    for (int i = 0; i < 200 && !trimmed_ids.isEmpty(); i++) {
      working_subset.append(trimmed_ids.takeFirst());
    }

    QString batch_final_url = target_url + working_subset.join(QL1C('&'));

    // We send this batch.
    if (async) {

      NetworkFactory::performAsyncNetworkOperation(batch_final_url,
                                                   timeout,
                                                   QByteArray(),
                                                   QNetworkAccessManager::Operation::GetOperation,
                                                   headers);
    }
    else {
      QByteArray output;

      NetworkFactory::performNetworkOperation(batch_final_url,
                                              timeout,
                                              QByteArray(),
                                              output,
                                              QNetworkAccessManager::Operation::GetOperation,
                                              headers);
    }

    // Cleanup for next batch.
    working_subset.clear();
  }
}

void GmailNetworkFactory::markMessagesStarred(RootItem::Importance importance, const QStringList& custom_ids, bool async) {
  QString target_url; // TODO: dodělat
  //= INOREADER_API_EDIT_TAG;

/*
   if (importance == RootItem::Importance::Important) {
    target_url += QString("?a=user/-/") + INOREADER_STATE_IMPORTANT + "&";
   }
   else {
    target_url += QString("?r=user/-/") + INOREADER_STATE_IMPORTANT + "&";
   }*/
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return;
  }

  QList<QPair<QByteArray, QByteArray>> headers;
  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));

  QStringList trimmed_ids;
  QRegularExpression regex_short_id(QSL("[0-9a-zA-Z]+$"));

  foreach (const QString& id, custom_ids) {
    QString simplified_id = regex_short_id.match(id).captured();

    trimmed_ids.append(QString("i=") + simplified_id);
  }

  QStringList working_subset;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  working_subset.reserve(trimmed_ids.size() > 200 ? 200 : trimmed_ids.size());

  // Now, we perform messages update in batches (max 200 messages per batch).
  while (!trimmed_ids.isEmpty()) {
    // We take 200 IDs.
    for (int i = 0; i < 200 && !trimmed_ids.isEmpty(); i++) {
      working_subset.append(trimmed_ids.takeFirst());
    }

    QString batch_final_url = target_url + working_subset.join(QL1C('&'));

    // We send this batch.
    if (async) {

      NetworkFactory::performAsyncNetworkOperation(batch_final_url,
                                                   timeout,
                                                   QByteArray(),
                                                   QNetworkAccessManager::Operation::GetOperation,
                                                   headers);
    }
    else {
      QByteArray output;

      NetworkFactory::performNetworkOperation(batch_final_url,
                                              timeout,
                                              QByteArray(),
                                              output,
                                              QNetworkAccessManager::Operation::GetOperation,
                                              headers);
    }

    // Cleanup for next batch.
    working_subset.clear();
  }
}

void GmailNetworkFactory::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(tr("Inoreader: authentication error"),
                       tr("Click this to login again. Error is: '%1'").arg(error_description),
                       QSystemTrayIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth2->login();
  });
}

void GmailNetworkFactory::onAuthFailed() {
  qApp->showGuiMessage(tr("Inoreader: authorization denied"),
                       tr("Click this to login again."),
                       QSystemTrayIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth2->login();
  });
}

QList<Message> GmailNetworkFactory::decodeMessages(const QString& messages_json_data, const QString& stream_id) {
  QList<Message> messages;
  QJsonArray json = QJsonDocument::fromJson(messages_json_data.toUtf8()).object()["items"].toArray();

  messages.reserve(json.count());

  foreach (const QJsonValue& obj, json) {
    auto message_obj = obj.toObject();
    Message message;

    message.m_title = message_obj["title"].toString();
    message.m_author = message_obj["author"].toString();
    message.m_created = QDateTime::fromSecsSinceEpoch(message_obj["published"].toInt());
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

      // TODO: dodělat
      //

      /*
         if (category.contains(INOREADER_STATE_READ)) {
         message.m_isRead = !category.contains(INOREADER_STATE_READING_LIST);
         }
         else if (category.contains(INOREADER_STATE_IMPORTANT)) {
         message.m_isImportant = category.contains(INOREADER_STATE_IMPORTANT);
         }*/
    }

    message.m_contents = message_obj["summary"].toObject()["content"].toString();
    message.m_feedId = stream_id;

    messages.append(message);
  }

  return messages;
}

RootItem* GmailNetworkFactory::decodeFeedCategoriesData(const QString& categories, const QString& feeds, bool obtain_icons) {
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
    GmailFeed* feed = new GmailFeed();

    feed->setDescription(url);
    feed->setUrl(url);
    feed->setTitle(title);
    feed->setCustomId(id);

    if (obtain_icons) {
      QString icon_url = subscription["iconUrl"].toString();

      if (!icon_url.isEmpty()) {
        QByteArray icon_data;

        if (NetworkFactory::performNetworkOperation(icon_url, DOWNLOAD_TIMEOUT,
                                                    QByteArray(), icon_data,
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
