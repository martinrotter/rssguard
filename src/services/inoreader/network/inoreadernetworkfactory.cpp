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
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"
#include "services/inoreader/definitions.h"

#include <QOAuthHttpServerReplyHandler>
#include <QUrl>

InoreaderNetworkFactory::InoreaderNetworkFactory(QObject* parent) : QObject(parent) {
  initializeOauth();
}

bool InoreaderNetworkFactory::isLoggedIn() const {
  return m_oauth2.expirationAt() > QDateTime::currentDateTime() && m_oauth2.status() == QAbstractOAuth::Status::Granted;
}

void InoreaderNetworkFactory::logIn() {
  if (!m_oauth2.expirationAt().isNull() && m_oauth2.expirationAt() <= QDateTime::currentDateTime()) {
    m_oauth2.refreshAccessToken();
  }
  else {
    m_oauth2.grant();
  }
}

void InoreaderNetworkFactory::logInIfNeeded() {
  if (!isLoggedIn()) {
    logIn();
  }
}

void InoreaderNetworkFactory::initializeOauth() {
  auto oauth_reply_handler = new QOAuthHttpServerReplyHandler(INOREADER_OAUTH_PORT, this);

  // Full redirect URL is thus "http://localhost.8080/".
  oauth_reply_handler->setCallbackPath(QSL(""));

  m_oauth2.setAccessTokenUrl(QUrl(INOREADER_OAUTH_TOKEN_URL));
  m_oauth2.setAuthorizationUrl(QUrl(INOREADER_OAUTH_AUTH_URL));
  m_oauth2.setClientIdentifier(INOREADER_OAUTH_CLI_ID);
  m_oauth2.setClientIdentifierSharedKey(INOREADER_OAUTH_CLI_KEY);
  m_oauth2.setContentType(QAbstractOAuth::ContentType::Json);
  m_oauth2.setNetworkAccessManager(SilentNetworkAccessManager::instance());
  m_oauth2.setReplyHandler(oauth_reply_handler);
  m_oauth2.setUserAgent(APP_USERAGENT);
  m_oauth2.setScope(INOREADER_OAUTH_SCOPE);

  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](QAbstractOAuth::Status status) {
    qDebug("Inoreader: Status changed to '%d'.", (int)status);
  });
  m_oauth2.setModifyParametersFunction([&](QAbstractOAuth::Stage stage, QVariantMap* parameters) {
    qDebug() << "Inoreader: Set modify parameters for stage" << (int)stage << "called: \n" << parameters;
  });
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::granted, [=]() {
    qDebug("Inoreader: Oauth2 granted.");
  });
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::error, [](const QString& error, const QString& error_description, const QUrl& uri) {
    Q_UNUSED(error)
    Q_UNUSED(uri)

    qCritical("Inoreader: We have error: '%s'.", qPrintable(error_description));
  });
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, [](const QUrl& url) {
    qApp->web()->openUrlInExternalBrowser(url.toString());
  });
}
