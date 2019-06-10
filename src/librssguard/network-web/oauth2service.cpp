// For license of this file, see <project-root-folder>/LICENSE.md.

////////////////////////////////////////////////////////////////////////////////

//                                                                            //
// This file is part of QOAuth2.                                              //
// Copyright (c) 2014 Jacob Dawid <jacob@omg-it.works>                        //
//                                                                            //
// QOAuth2 is free software: you can redistribute it and/or modify            //
// it under the terms of the GNU Affero General Public License as             //
// published by the Free Software Foundation, either version 3 of the         //
// License, or (at your option) any later version.                            //
//                                                                            //
// QOAuth2 is distributed in the hope that it will be useful,                 //
// but WITHOUT ANY WARRANTY; without even the implied warranty of             //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              //
// GNU Affero General Public License for more details.                        //
//                                                                            //
// You should have received a copy of the GNU Affero General Public           //
// License along with QOAuth2.                                                //
// If not, see <http://www.gnu.org/licenses/>.                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "network-web/oauth2service.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/inoreader/definitions.h"

#if defined(USE_WEBENGINE)
#include "gui/dialogs/oauthlogin.h"
#else
#include "network-web/oauthhttphandler.h"

Q_GLOBAL_STATIC(OAuthHttpHandler, qz_silent_acmanager)
#endif

#include <cstdlib>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <utility>

OAuth2Service::OAuth2Service(const QString& auth_url, const QString& token_url, const QString& client_id,
                             const QString& client_secret, const QString& scope, QObject* parent)
  : QObject(parent), m_id(QString::number(std::rand())), m_timerId(-1) {
  m_redirectUrl = QSL(LOCALHOST_ADDRESS);
  m_tokenGrantType = QSL("authorization_code");
  m_tokenUrl = QUrl(token_url);
  m_authUrl = auth_url;

  m_clientId = client_id;
  m_clientSecret = client_secret;
  m_scope = scope;

  connect(&m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(tokenRequestFinished(QNetworkReply*)));

#if !defined(USE_WEBENGINE)
  connect(handler(), &OAuthHttpHandler::authGranted, [this](const QString& auth_code, const QString& id) {
    if (id.isEmpty() || id == m_id) {
      // We process this further only if handler (static singleton) responded to our original request.
      retrieveAccessToken(auth_code);
    }
  });
  connect(handler(), &OAuthHttpHandler::authRejected, [this](const QString& error_description, const QString& id) {
    Q_UNUSED(error_description)

    if (id.isEmpty() || id == m_id) {
      // We process this further only if handler (static singleton) responded to our original request.
      emit authFailed();
    }
  });
#endif
}

QString OAuth2Service::bearer() {
  if (!isFullyLoggedIn()) {
    qApp->showGuiMessage(tr("You have to login first"),
                         tr("Click here to login."),
                         QSystemTrayIcon::Critical,
                         nullptr, false,
                         [this]() {
      login();
    });
    return QString();
  }
  else {
    return QString("Bearer %1").arg(accessToken());
  }
}

bool OAuth2Service::isFullyLoggedIn() const {
  bool is_expiration_valid = tokensExpireIn() > QDateTime::currentDateTime();
  bool do_tokens_exist = !refreshToken().isEmpty() && !accessToken().isEmpty();

  return is_expiration_valid && do_tokens_exist;
}

void OAuth2Service::setOAuthTokenGrantType(QString grant_type) {
  m_tokenGrantType = std::move(grant_type);
}

QString OAuth2Service::oAuthTokenGrantType() {
  return m_tokenGrantType;
}

void OAuth2Service::timerEvent(QTimerEvent* event) {
  if (m_timerId >= 0 && event->timerId() == m_timerId) {
    event->accept();

    QDateTime window_about_expire = tokensExpireIn().addSecs(-60 * 15);

    if (window_about_expire < QDateTime::currentDateTime()) {
      // We try to refresh access token, because it probably expires soon.
      qDebug("Refreshing automatically access token.");
      refreshAccessToken();
    }
    else {
      qDebug("Access token is not expired yet.");
    }
  }

  QObject::timerEvent(event);
}

QString OAuth2Service::id() const {
  return m_id;
}

void OAuth2Service::setId(const QString& id) {
  m_id = id;
}

#if !defined(USE_WEBENGINE)
OAuthHttpHandler* OAuth2Service::handler() {
  return qz_silent_acmanager();
}

#endif

void OAuth2Service::retrieveAccessToken(const QString& auth_code) {
  QNetworkRequest networkRequest;

  networkRequest.setUrl(m_tokenUrl);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QString content = QString("client_id=%1&"
                            "client_secret=%2&"
                            "code=%3&"
                            "redirect_uri=%5&"
                            "grant_type=%4").arg(m_clientId, m_clientSecret, auth_code, m_tokenGrantType, m_redirectUrl);

  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::refreshAccessToken(QString refresh_token) {
  if (refresh_token.isEmpty()) {
    refresh_token = refreshToken();
  }

  QNetworkRequest networkRequest;

  networkRequest.setUrl(m_tokenUrl);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QString content = QString("client_id=%1&"
                            "client_secret=%2&"
                            "refresh_token=%3&"
                            "grant_type=%4").arg(m_clientId, m_clientSecret, refresh_token, QSL("refresh_token"));

  qApp->showGuiMessage(tr("Logging in via OAuth 2.0..."),
                       tr("Refreshing login tokens for '%1'...").arg(m_tokenUrl.toString()),
                       QSystemTrayIcon::MessageIcon::Information);

  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::tokenRequestFinished(QNetworkReply* network_reply) {
  QByteArray repl = network_reply->readAll();
  QJsonDocument json_document = QJsonDocument::fromJson(repl);
  QJsonObject root_obj = json_document.object();

  qDebug() << "Token response:" << json_document.toJson();

  if (network_reply->error() != QNetworkReply::NetworkError::NoError) {
    emit tokensRetrieveError(QString(), NetworkFactory::networkErrorText(network_reply->error()));
  }
  else if (root_obj.keys().contains("error")) {
    QString error = root_obj.value("error").toString();
    QString error_description = root_obj.value("error_description").toString();

    logout();

    emit tokensRetrieveError(error, error_description);
  }
  else {
    int expires = root_obj.value(QL1S("expires_in")).toInt();

    setTokensExpireIn(QDateTime::currentDateTime().addSecs(expires));
    setAccessToken(root_obj.value(QL1S("access_token")).toString());

    const QString refresh_token = root_obj.value(QL1S("refresh_token")).toString();

    if (!refresh_token.isEmpty()) {
      setRefreshToken(refresh_token);
    }

    qDebug() << "Obtained refresh token" << refreshToken() << "- expires on date/time" << tokensExpireIn();

    emit tokensReceived(accessToken(), refreshToken(), expires);
  }

  network_reply->deleteLater();
}

QString OAuth2Service::accessToken() const {
  return m_accessToken;
}

void OAuth2Service::setAccessToken(const QString& access_token) {
  m_accessToken = access_token;
}

QDateTime OAuth2Service::tokensExpireIn() const {
  return m_tokensExpireIn;
}

void OAuth2Service::setTokensExpireIn(const QDateTime& tokens_expire_in) {
  m_tokensExpireIn = tokens_expire_in;
}

QString OAuth2Service::clientSecret() const {
  return m_clientSecret;
}

void OAuth2Service::setClientSecret(const QString& client_secret) {
  m_clientSecret = client_secret;
}

QString OAuth2Service::clientId() const {
  return m_clientId;
}

void OAuth2Service::setClientId(const QString& client_id) {
  m_clientId = client_id;
}

QString OAuth2Service::redirectUrl() const {
  return m_redirectUrl;
}

void OAuth2Service::setRedirectUrl(const QString& redirect_url) {
  m_redirectUrl = redirect_url;
}

QString OAuth2Service::refreshToken() const {
  return m_refreshToken;
}

void OAuth2Service::setRefreshToken(const QString& refresh_token) {
  killRefreshTimer();
  m_refreshToken = refresh_token;
  startRefreshTimer();
}

bool OAuth2Service::login() {
  bool did_token_expire = tokensExpireIn().isNull() || tokensExpireIn() < QDateTime::currentDateTime();
  bool does_token_exist = !refreshToken().isEmpty();

  // We refresh current tokens only if:
  //   1. We have some existing refresh token.
  //   AND
  //   2. We do not know its expiration date or it passed.
  if (does_token_exist && did_token_expire) {
    refreshAccessToken();
    return false;
  }
  else if (!does_token_exist) {
    retrieveAuthCode();
    return false;
  }
  else {
    return true;
  }
}

void OAuth2Service::logout() {
  setTokensExpireIn(QDateTime());
  setAccessToken(QString());
  setRefreshToken(QString());
}

void OAuth2Service::startRefreshTimer() {
  if (!refreshToken().isEmpty()) {
    m_timerId = startTimer(1000 * 60 * 15, Qt::VeryCoarseTimer);
  }
}

void OAuth2Service::killRefreshTimer() {
  if (m_timerId > 0) {
    killTimer(m_timerId);
  }
}

void OAuth2Service::retrieveAuthCode() {
  QString auth_url = m_authUrl + QString("?client_id=%1&scope=%2&"
                                         "redirect_uri=%3&response_type=code&state=%4&"
                                         "prompt=consent&access_type=offline").arg(m_clientId,
                                                                                   m_scope,
                                                                                   m_redirectUrl,
                                                                                   m_id);

#if defined(USE_WEBENGINE)
  OAuthLogin login_page(qApp->mainFormWidget());

  connect(&login_page, &OAuthLogin::authGranted, this, &OAuth2Service::retrieveAccessToken);
  connect(&login_page, &OAuthLogin::authRejected, this, [this]() {
    logout();
    emit authFailed();
  });

  qApp->showGuiMessage(tr("Logging in via OAuth 2.0..."),
                       tr("Requesting access authorization for '%1'...").arg(m_authUrl),
                       QSystemTrayIcon::MessageIcon::Information);

  login_page.login(auth_url, m_redirectUrl);
#else

  // We run login URL in external browser, response is caught by light HTTP server.
  qApp->web()->openUrlInExternalBrowser(auth_url);
#endif
}
