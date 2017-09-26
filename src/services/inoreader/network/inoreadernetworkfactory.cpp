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

bool InoreaderNetworkFactory::isLoggedIn() const {
  return !m_oauth2->refreshToken().isEmpty();
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

void InoreaderNetworkFactory::login() {
  m_oauth2->login();
}

void InoreaderNetworkFactory::initializeOauth() {
  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, [](QString error, QString error_description) {
    qApp->showGuiMessage("Authentication error - Inoreader", error_description, QSystemTrayIcon::Critical);
  });
}

void InoreaderNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

// NOTE: oauth: https://developers.google.com/oauthplayground/#step3&scopes=read%20write&auth_code=497815bc3362aba9ad60c5ae3e01811fe2da4bb5&refresh_token=bacb9c36f82ba92667282d6175bb857a091e7f0c&access_token_field=094f92bc7aedbd27fbebc3efc9172b258be8944a&url=https%3A%2F%2Fwww.inoreader.com%2Freader%2Fapi%2F0%2Fsubscription%2Flist&content_type=application%2Fjson&http_method=GET&useDefaultOauthCred=unchecked&oauthEndpointSelect=Custom&oauthAuthEndpointValue=https%3A%2F%2Fwww.inoreader.com%2Foauth2%2Fauth%3Fstate%3Dtest&oauthTokenEndpointValue=https%3A%2F%2Fwww.inoreader.com%2Foauth2%2Ftoken&oauthClientId=1000000595&expires_in=3599&oauthClientSecret=_6pYUZgtNLWwSaB9pC1YOz6p4zwu3haL&access_token_issue_date=1506198338&for_access_token=094f92bc7aedbd27fbebc3efc9172b258be8944a&includeCredentials=checked&accessTokenType=bearer&autoRefreshToken=unchecked&accessType=offline&prompt=consent&response_type=code

RootItem* InoreaderNetworkFactory::feedsCategories(bool obtain_icons) {
  RootItem* parent = new RootItem();

  QMap<QString, RootItem*> cats;
  cats.insert(QSL(""), parent);

  QNetworkRequest req(QUrl(INOREADER_API_LIST_LABELS));

  m_oauth2->attachBearerHeader(req);

  QNetworkReply* reply = SilentNetworkAccessManager::instance()->get(req);
  QEventLoop loop;

  connect(reply, &QNetworkReply::finished, [&]() {
    if (reply->error() == QNetworkReply::NoError) {
      QByteArray repl_data = reply->readAll();
      QJsonArray json = QJsonDocument::fromJson(repl_data).object()["tags"].toArray();

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

          if (obtain_icons) {
            QString icon_url = label["iconUrl"].toString();

            if (!icon_url.isEmpty()) {
              QByteArray icon_data;

              if (NetworkFactory::performNetworkOperation(icon_url, DOWNLOAD_TIMEOUT,
                                                          QByteArray(), QString(), icon_data,
                                                          QNetworkAccessManager::GetOperation).first == QNetworkReply::NoError) {
                // Icon downloaded, set it up.
                QPixmap icon_pixmap;

                icon_pixmap.loadFromData(icon_data);
                category->setIcon(QIcon(icon_pixmap));
              }
            }
          }

          // All categories in ownCloud are top-level.
          parent->appendChild(category);
        }
      }
    }

    loop.exit();
  });

  loop.exec();

  return parent;
}
