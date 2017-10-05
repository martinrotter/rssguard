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
#include "gui/dialogs/oauthlogin.h"
#include "miscellaneous/application.h"
#include "services/inoreader/definitions.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

OAuth2Service::OAuth2Service(QString authUrl, QString tokenUrl, QString clientId,
                             QString clientSecret, QString scope, QObject* parent)
  : QObject(parent), m_tokensExpireIn(QDateTime()) {

  m_redirectUrl = QSL(INOREADER_OAUTH_CLI_REDIRECT);
  m_tokenGrantType = QSL("authorization_code");
  m_tokenUrl = QUrl(tokenUrl);
  m_authUrl = authUrl;

  m_clientId = clientId;
  m_clientSecret = clientSecret;
  m_scope = scope;

  connect(&m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(tokenRequestFinished(QNetworkReply*)));
  connect(this, &OAuth2Service::authCodeObtained, this, &OAuth2Service::retrieveAccessToken);
}

QString OAuth2Service::bearer() {
  if (!isFullyLoggedIn()) {
    qApp->showGuiMessage(tr("Inoreader: you have to login first"),
                         tr("Click here to login."),
                         QSystemTrayIcon::Critical,
                         nullptr, false,
                         [this]() {
      login();
    });
    return QString();
  }
  else {
    return QString("Bearer %1").arg(m_accessToken);
  }
}

bool OAuth2Service::isFullyLoggedIn() const {
  bool is_expiration_valid = m_tokensExpireIn > QDateTime::currentDateTime();
  bool do_tokens_exist = !m_refreshToken.isEmpty() && !m_accessToken.isEmpty();

  return is_expiration_valid && do_tokens_exist;
}

void OAuth2Service::setOAuthTokenGrantType(QString grant_type) {
  m_tokenGrantType = grant_type;
}

QString OAuth2Service::oAuthTokenGrantType() {
  return m_tokenGrantType;
}

void OAuth2Service::retrieveAccessToken(QString auth_code) {
  QNetworkRequest networkRequest;

  networkRequest.setUrl(m_tokenUrl);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QString content = QString("client_id=%1&"
                            "client_secret=%2&"
                            "code=%3&"
                            "redirect_uri=%5&"
                            "grant_type=%4")
                    .arg(m_clientId)
                    .arg(m_clientSecret)
                    .arg(auth_code)
                    .arg(m_tokenGrantType)
                    .arg(m_redirectUrl);

  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::refreshAccessToken(QString refresh_token) {
  if (refresh_token.isEmpty()) {
    refresh_token = m_refreshToken;
  }

  QNetworkRequest networkRequest;

  networkRequest.setUrl(m_tokenUrl);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QString content = QString("client_id=%1&"
                            "client_secret=%2&"
                            "refresh_token=%3&"
                            "grant_type=%4")
                    .arg(m_clientId)
                    .arg(m_clientSecret)
                    .arg(refresh_token)
                    .arg("refresh_token");

  qApp->showGuiMessage(tr("Logging in via OAuth 2.0..."),
                       tr("Refreshing login tokens for '%1'...").arg(m_tokenUrl.toString()),
                       QSystemTrayIcon::MessageIcon::Information);

  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::tokenRequestFinished(QNetworkReply* network_reply) {
  QJsonDocument jsonDocument = QJsonDocument::fromJson(network_reply->readAll());
  QJsonObject rootObject = jsonDocument.object();

  qDebug() << "Token response:";
  qDebug() << jsonDocument.toJson();

  if (rootObject.keys().contains("error")) {
    QString error = rootObject.value("error").toString();
    QString error_description = rootObject.value("error_description").toString();

    logout();

    emit tokensRetrieveError(error, error_description);
  }
  else {
    int expires = rootObject.value(QL1S("expires_in")).toInt();

    m_accessToken = rootObject.value(QL1S("access_token")).toString();
    m_refreshToken = rootObject.value(QL1S("refresh_token")).toString();
    m_tokensExpireIn = QDateTime::currentDateTime().addSecs(expires);

    qDebug() << "Obtained refresh token" << m_refreshToken << "- expires on date/time" << m_tokensExpireIn;

    // TODO: Start timer to refresh tokens?
    emit tokensReceived(m_accessToken, m_refreshToken, rootObject.value("expires_in").toInt());
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
  m_refreshToken = refresh_token;
}

bool OAuth2Service::login() {
  bool did_token_expire = m_tokensExpireIn.isNull() || m_tokensExpireIn < QDateTime::currentDateTime();
  bool does_token_exist = !m_refreshToken.isEmpty();

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
  m_refreshToken = m_accessToken = QString();
  m_tokensExpireIn = QDateTime();

  // TODO: zastavit timer na obnovení refresh tokenu?
}

void OAuth2Service::retrieveAuthCode() {
  QString auth_url = m_authUrl + QString("?client_id=%1&scope=%2&"
                                         "redirect_uri=%3&response_type=code&state=abcdef").arg(m_clientId,
                                                                                                m_scope,
                                                                                                m_redirectUrl);
  OAuthLogin login_page(qApp->mainFormWidget());

  connect(&login_page, &OAuthLogin::authGranted, this, &OAuth2Service::authCodeObtained);
  connect(&login_page, &OAuthLogin::authRejected, [this]() {
    logout();
    emit authFailed();
  });

  qApp->showGuiMessage(tr("Logging in via OAuth 2.0..."),
                       tr("Requesting access authorization for '%1'...").arg(m_authUrl),
                       QSystemTrayIcon::MessageIcon::Information);

  login_page.login(auth_url, m_redirectUrl);
}
