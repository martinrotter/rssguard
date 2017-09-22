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

InoreaderNetworkFactory::InoreaderNetworkFactory(QObject* parent) : QObject(parent),
  m_batchSize(INOREADER_DEFAULT_BATCH_SIZE), m_oauth2(new QOAuth2AuthorizationCodeFlow(this)) {
  initializeOauth();
}

bool InoreaderNetworkFactory::isLoggedIn() const {
  return m_oauth2->expirationAt() > QDateTime::currentDateTime() && m_oauth2->status() == QAbstractOAuth::Status::Granted;
}

QString InoreaderNetworkFactory::username() const {
  return m_username;
}

int InoreaderNetworkFactory::batchSize() const {
  return m_batchSize;
}

void InoreaderNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

void InoreaderNetworkFactory::logIn() {
  if (!m_oauth2->expirationAt().isNull() && m_oauth2->expirationAt() <= QDateTime::currentDateTime() && !m_refreshToken.isEmpty()) {
    // We have some refresh token which expired.
    m_oauth2->refreshAccessToken();
  }
  else {
    m_oauth2->grant();
  }
}

void InoreaderNetworkFactory::logInIfNeeded() {
  if (!isLoggedIn()) {
    logIn();
  }
}

void InoreaderNetworkFactory::tokensReceived(QVariantMap tokens) {
  qDebug() << "Inoreader: Tokens received:" << tokens;

  if (tokens.contains(INOREADER_ACCESS_TOKEN_KEY)) {
    m_accessToken = tokens.value(INOREADER_ACCESS_TOKEN_KEY).toString();
  }

  if (tokens.contains(INOREADER_REFRESH_TOKEN_KEY)) {
    m_refreshToken = tokens.value(INOREADER_REFRESH_TOKEN_KEY).toString();
  }

  emit tokensRefreshed();
}

void InoreaderNetworkFactory::initializeOauth() {
  auto oauth_reply_handler = new QOAuthHttpServerReplyHandler(INOREADER_OAUTH_PORT, this);

  // Full redirect URL is thus "http://localhost:INOREADER_OAUTH_PORT/".
  oauth_reply_handler->setCallbackPath(QSL(""));
  oauth_reply_handler->setCallbackText(tr("Access to your Inoreader session was granted, you "
                                          "can now <b>close this window and go back to RSS Guard</b>."));

  m_oauth2->setAccessTokenUrl(QUrl(INOREADER_OAUTH_TOKEN_URL));
  m_oauth2->setAuthorizationUrl(QUrl(INOREADER_OAUTH_AUTH_URL));
  m_oauth2->setClientIdentifier(INOREADER_OAUTH_CLI_ID);
  m_oauth2->setClientIdentifierSharedKey(INOREADER_OAUTH_CLI_KEY);
  m_oauth2->setContentType(QAbstractOAuth::ContentType::Json);
  m_oauth2->setNetworkAccessManager(SilentNetworkAccessManager::instance());
  m_oauth2->setReplyHandler(oauth_reply_handler);
  m_oauth2->setUserAgent(APP_USERAGENT);
  m_oauth2->setScope(INOREADER_OAUTH_SCOPE);

  connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](QAbstractOAuth::Status status) {
    qDebug("Inoreader: Status changed to '%d'.", (int)status);
  });
  connect(oauth_reply_handler, &QOAuthHttpServerReplyHandler::tokensReceived, this, &InoreaderNetworkFactory::tokensReceived);
  m_oauth2->setModifyParametersFunction([&](QAbstractOAuth::Stage stage, QVariantMap* parameters) {
    qDebug() << "Inoreader: Set modify parameters for stage" << (int)stage << "called: \n" << parameters;
  });
  connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::granted, [=]() {
    qDebug("Inoreader: Oauth2 granted.");
    emit accessGranted();
  });
  connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::error, [=](QString err, QString error_description, QUrl uri) {
    Q_UNUSED(err)
    Q_UNUSED(uri)

    qCritical("Inoreader: We have error: '%s'.", qPrintable(error_description));
    m_accessToken = m_refreshToken = QString();
    emit error(error_description);
  });
  connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, [](const QUrl& url) {
    qApp->web()->openUrlInExternalBrowser(url.toString());
  });
}

QString InoreaderNetworkFactory::refreshToken() const {
  return m_refreshToken;
}

QString InoreaderNetworkFactory::accessToken() const {
  return m_accessToken;
}
