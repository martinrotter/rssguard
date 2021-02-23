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

#ifndef OAUTH2SERVICE_H
#define OAUTH2SERVICE_H

#include <QObject>
#include <QUrl>

#include "network-web/oauthhttphandler.h"
#include "network-web/silentnetworkaccessmanager.h"

class OAuth2Service : public QObject {
  Q_OBJECT

  public:
    explicit OAuth2Service(const QString& auth_url, const QString& token_url,
                           const QString& client_id, const QString& client_secret,
                           const QString& scope, QObject* parent = nullptr);
    virtual ~OAuth2Service();

    // Returns bearer HTTP header value.
    // NOTE: If on working thread, then call this only if isFullyLoggedIn()
    // returns true. If isFullyLoggedIn() returns false, then you must call login() on
    // main GUI thread first.
    QString bearer();
    bool isFullyLoggedIn() const;

    void setOAuthTokenGrantType(QString grant_type);
    QString oAuthTokenGrantType();

    QString refreshToken() const;
    void setRefreshToken(const QString& refresh_token);

    QString redirectUrl() const;
    void setRedirectUrl(const QString& redirect_url);

    QString clientId() const;
    void setClientId(const QString& client_id);

    QString clientSecret() const;
    void setClientSecret(const QString& client_secret);

    QDateTime tokensExpireIn() const;
    void setTokensExpireIn(const QDateTime& tokens_expire_in);

    QString accessToken() const;
    void setAccessToken(const QString& access_token);

    QString id() const;
    void setId(const QString& id);

  signals:
    void tokensRetrieved(QString access_token, QString refresh_token, int expires_in);
    void tokensRetrieveError(QString error, QString error_description);

    // User failed to authenticate or rejected it.
    void authFailed();

  public slots:
    void retrieveAuthCode();
    void retrieveAccessToken(const QString& auth_code);
    void refreshAccessToken(const QString& refresh_token = QString());

    // Performs login if needed. If some refresh token is set, then
    // the initial "auth" step is skipped and attempt to refresh
    // access token is made.
    //
    // Returns true, if user is already logged in (final state).
    // Returns false, if user is NOT logged in (asynchronous flow).
    //
    // NOTE: This can be called ONLY on main GUI thread,
    // because widgets may be displayed.
    bool login();

    // Removes all state data and stops redirection handler.
    void logout(bool stop_redirection_handler = true);

  private slots:
    void startRefreshTimer();
    void killRefreshTimer();
    void tokenRequestFinished(QNetworkReply* network_reply);

  private:
    void timerEvent(QTimerEvent* event);

  private:
    QString m_id;
    int m_timerId;
    QDateTime m_tokensExpireIn;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_tokenGrantType;
    QString m_clientId;
    QString m_clientSecret;
    QUrl m_tokenUrl;
    QString m_authUrl;
    QString m_scope;
    SilentNetworkAccessManager m_networkManager;
    OAuthHttpHandler* m_redirectionHandler;
};

#endif // OAUTH2SERVICE_H
