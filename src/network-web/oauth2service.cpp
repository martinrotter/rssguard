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

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

OAuth2Service::OAuth2Service(QString authUrl, QString tokenUrl, QString clientId,
                             QString clientSecret, QString scope, QObject* parent)
  : QObject(parent) {
  m_redirectUri = QSL("http://localhost");
  m_tokenGrantType = QSL("authorization_code");
  m_tokenUrl = QUrl(tokenUrl);
  m_authUrl = authUrl;

  m_clientId = clientId;
  m_clientSecret = clientSecret;
  m_scope = scope;

  connect(&m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(tokenRequestFinished(QNetworkReply*)));
  connect(this, &OAuth2Service::authCodeObtained, this, &OAuth2Service::retrieveAccessToken);
}

void OAuth2Service::setBearerHeader(QNetworkRequest& req) {
  req.setRawHeader(QString("Authorization").toLocal8Bit(), QString("Bearer %1").arg(m_accessToken).toLocal8Bit());
}

void OAuth2Service::setOAuthTokenGrantType(QString oAuthTokenGrantType) {
  m_tokenGrantType = oAuthTokenGrantType;
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
                    .arg(m_redirectUri);

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

  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::tokenRequestFinished(QNetworkReply* networkReply) {
  QJsonDocument jsonDocument = QJsonDocument::fromJson(networkReply->readAll());
  QJsonObject rootObject = jsonDocument.object();

  qDebug() << "Token response:";
  qDebug() << jsonDocument.toJson();

  if(rootObject.keys().contains("error")) {
    QString error = rootObject.value("error").toString();
    QString error_description = rootObject.value("error_description").toString();
    emit tokenRetrieveError(error, error_description);
  }
  else {
    m_accessToken = rootObject.value("access_token").toString();
    m_refreshToken = rootObject.value("refresh_token").toString();

    // TODO: Start timer to refresh tokens.

    emit accessTokenReceived(m_accessToken, m_refreshToken, rootObject.value("expires_in").toInt());
  }

  networkReply->deleteLater();
}

QString OAuth2Service::refreshToken() const {
  return m_refreshToken;
}

void OAuth2Service::setRefreshToken(const QString& refresh_token) {
  m_refreshToken = refresh_token;
}

void OAuth2Service::retrieveAuthCode() {
  QString auth_url = m_authUrl + QString("?client_id=%1&scope=%2&"
                                         "redirect_uri=%3&response_type=code&state=abcdef").arg(m_clientId,
                                                                                                m_scope,
                                                                                                m_redirectUri);
  OAuthLogin login_page(qApp->mainFormWidget());

  connect(&login_page, &OAuthLogin::authGranted, this, &OAuth2Service::authCodeObtained);
  connect(&login_page, &OAuthLogin::authRejected, this, &OAuth2Service::authFailed);
  login_page.login(auth_url, m_redirectUri);
}

QString OAuth2Service::accessToken() const {
  return m_accessToken;
}

void OAuth2Service::setAccessToken(const QString& access_token) {
  m_accessToken = access_token;
}
