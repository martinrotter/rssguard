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
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"
#include "network-web/oauthhttphandler.h"
#include "network-web/webfactory.h"

#include <QDebug>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>

#include <cstdlib>
#include <utility>

OAuth2Service::OAuth2Service(const QString& auth_url, const QString& token_url, const QString& client_id,
                             const QString& client_secret, const QString& scope, QObject* parent)
  : QObject(parent),
  m_id(QString::number(QRandomGenerator::global()->generate())), m_timerId(-1),
  m_redirectionHandler(new OAuthHttpHandler(tr("You can close this window now. Go back to %1.").arg(QSL(APP_NAME)), this)),
  m_functorOnLogin(std::function<void()>()) {
  m_tokenGrantType = QSL("authorization_code");
  m_tokenUrl = QUrl(token_url);
  m_authUrl = auth_url;

  m_clientId = client_id;
  m_clientSecret = client_secret;
  m_clientSecretId = m_clientSecretSecret = QString();
  m_scope = scope;

  connect(&m_networkManager, &QNetworkAccessManager::finished, this, &OAuth2Service::tokenRequestFinished);
  connect(m_redirectionHandler, &OAuthHttpHandler::authGranted, [this](const QString& auth_code, const QString& id) {
    if (id.isEmpty() || id == m_id) {
      // We process this further only if handler (static singleton) responded to our original request.
      retrieveAccessToken(auth_code);
    }
  });
  connect(m_redirectionHandler, &OAuthHttpHandler::authRejected, [this](const QString& error_description, const QString& id) {
    Q_UNUSED(error_description)

    if (id.isEmpty() || id == m_id) {
      // We process this further only if handler (static singleton) responded to our original request.
      emit authFailed();
    }
  });
}

OAuth2Service::~OAuth2Service() {
  qDebugNN << LOGSEC_OAUTH << "Destroying OAuth2Service instance.";
}

QString OAuth2Service::bearer() {
  if (!isFullyLoggedIn()) {
    qApp->showGuiMessage(Notification::Event::LoginFailure,
                         tr("You have to login first"),
                         tr("Click here to login."),
                         QSystemTrayIcon::MessageIcon::Critical,
                         {}, {},
                         tr("Login"),
                         [this]() {
      login();
    });
    return {};
  }
  else {
    return QSL("Bearer %1").arg(accessToken());
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
      qDebugNN << LOGSEC_OAUTH << "Refreshing automatically access token.";
      refreshAccessToken();
    }
    else {
      qDebugNN << LOGSEC_OAUTH << "Access token is not expired yet.";
    }
  }

  QObject::timerEvent(event);
}

QString OAuth2Service::clientSecretSecret() const {
  return m_clientSecretSecret;
}

void OAuth2Service::setClientSecretSecret(const QString& client_secret_secret) {
  m_clientSecretSecret = client_secret_secret;
}

QString OAuth2Service::clientSecretId() const {
  return m_clientSecretId;
}

void OAuth2Service::setClientSecretId(const QString& client_secret_id) {
  m_clientSecretId = client_secret_id;
}

QString OAuth2Service::id() const {
  return m_id;
}

void OAuth2Service::setId(const QString& id) {
  m_id = id;
}

void OAuth2Service::retrieveAccessToken(const QString& auth_code) {
  QNetworkRequest networkRequest;

  networkRequest.setUrl(m_tokenUrl);
  networkRequest.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");

  QString content = QString("client_id=%1&"
                            "client_secret=%2&"
                            "code=%3&"
                            "redirect_uri=%5&"
                            "grant_type=%4").arg(properClientId(),
                                                 properClientSecret(),
                                                 auth_code,
                                                 m_tokenGrantType,
                                                 m_redirectionHandler->listenAddressPort());

  qDebugNN << LOGSEC_OAUTH << "Posting data for access token retrieval:" << QUOTE_W_SPACE_DOT(content);
  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::refreshAccessToken(const QString& refresh_token) {
  auto real_refresh_token = refresh_token.isEmpty() ? refreshToken() : refresh_token;
  QNetworkRequest networkRequest;

  networkRequest.setUrl(m_tokenUrl);
  networkRequest.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");

  QString content = QString("client_id=%1&"
                            "client_secret=%2&"
                            "refresh_token=%3&"
                            "grant_type=%4").arg(properClientId(),
                                                 properClientSecret(),
                                                 real_refresh_token,
                                                 QSL("refresh_token"));

  qApp->showGuiMessage(Notification::Event::LoginDataRefreshed,
                       tr("Logging in via OAuth 2.0..."),
                       tr("Refreshing login tokens for '%1'...").arg(m_tokenUrl.toString()),
                       QSystemTrayIcon::MessageIcon::Information);

  qDebugNN << LOGSEC_OAUTH << "Posting data for access token refreshing:" << QUOTE_W_SPACE_DOT(content);
  m_networkManager.post(networkRequest, content.toUtf8());
}

void OAuth2Service::tokenRequestFinished(QNetworkReply* network_reply) {
  QByteArray repl = network_reply->readAll();
  QJsonDocument json_document = QJsonDocument::fromJson(repl);
  QJsonObject root_obj = json_document.object();

  qDebugNN << LOGSEC_OAUTH << "Token response:" << QUOTE_W_SPACE_DOT(QString::fromUtf8(json_document.toJson()));

  if (network_reply->error() != QNetworkReply::NetworkError::NoError) {
    qWarningNN << LOGSEC_OAUTH
               << "Network error when obtaining token response:"
               << QUOTE_W_SPACE_DOT(network_reply->error());

    emit tokensRetrieveError(QString(), NetworkFactory::networkErrorText(network_reply->error()));
  }
  else if (root_obj.keys().contains(QSL("error"))) {
    QString error = root_obj.value(QSL("error")).toString();
    QString error_description = root_obj.value(QSL("error_description")).toString();

    qWarningNN << LOGSEC_OAUTH
               << "JSON error when obtaining token response:"
               << QUOTE_W_SPACE(error)
               << QUOTE_W_SPACE_DOT(error_description);

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

    qDebugNN << LOGSEC_OAUTH
             << "Obtained refresh token" << QUOTE_W_SPACE(refreshToken())
             << "- expires on date/time" << QUOTE_W_SPACE_DOT(tokensExpireIn());

    if (m_functorOnLogin != nullptr) {
      m_functorOnLogin();
    }

    emit tokensRetrieved(accessToken(), refreshToken(), expires);
  }

  network_reply->deleteLater();
}

QString OAuth2Service::properClientId() const {
  return m_clientId.simplified().isEmpty()
      ? m_clientSecretId
      : m_clientId;
}

QString OAuth2Service::properClientSecret() const {
  return m_clientSecret.simplified().isEmpty()
      ? m_clientSecretSecret
      : m_clientSecret;
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
  return m_redirectionHandler->listenAddressPort();
}

void OAuth2Service::setRedirectUrl(const QString& redirect_url, bool start_handler) {
  m_redirectionHandler->setListenAddressPort(redirect_url, start_handler);
}

QString OAuth2Service::refreshToken() const {
  return m_refreshToken;
}

void OAuth2Service::setRefreshToken(const QString& refresh_token) {
  killRefreshTimer();
  m_refreshToken = refresh_token;
  startRefreshTimer();
}

bool OAuth2Service::login(const std::function<void()>& functor_when_logged_in) {
  m_functorOnLogin = functor_when_logged_in;

  if (!m_redirectionHandler->isListening()) {
    qCriticalNN << LOGSEC_OAUTH
                << "Cannot log-in because OAuth redirection handler is not listening.";

    emit tokensRetrieveError(QString(), tr("Failed to start OAuth "
                                           "redirection listener. Maybe your "
                                           "rights are not high enough."));

    return false;
  }

  bool did_token_expire = tokensExpireIn().isNull() || tokensExpireIn() < QDateTime::currentDateTime().addSecs(-120);
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
    functor_when_logged_in();
    return true;
  }
}

void OAuth2Service::logout(bool stop_redirection_handler) {
  setTokensExpireIn(QDateTime());
  setAccessToken(QString());
  setRefreshToken(QString());

  qDebugNN << LOGSEC_OAUTH << "Clearing tokens.";

  if (stop_redirection_handler) {
    m_redirectionHandler->stop();
  }
}

void OAuth2Service::startRefreshTimer() {
  if (!refreshToken().isEmpty()) {
    m_timerId = startTimer(1000 * 60 * 15, Qt::TimerType::VeryCoarseTimer);
  }
}

void OAuth2Service::killRefreshTimer() {
  if (m_timerId > 0) {
    killTimer(m_timerId);
  }
}

void OAuth2Service::retrieveAuthCode() {
  QString auth_url = m_authUrl + QString("?client_id=%1&"
                                         "scope=%2&"
                                         "redirect_uri=%3&"
                                         "response_type=code&"
                                         "state=%4&"
                                         "prompt=consent&"
                                         "access_type=offline").arg(properClientId(),
                                                                    m_scope,
                                                                    m_redirectionHandler->listenAddressPort(),
                                                                    m_id);

  // We run login URL in external browser, response is caught by light HTTP server.
  qApp->web()->openUrlInExternalBrowser(auth_url);
}
