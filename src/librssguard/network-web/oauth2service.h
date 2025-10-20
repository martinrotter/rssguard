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

#include "network-web/oauthhttphandler.h"
#include "network-web/silentnetworkaccessmanager.h"

#include <functional>

#include <QObject>
#include <QUrl>

class RSSGUARD_DLLSPEC OAuth2Service : public QObject {
    Q_OBJECT

  public:
    explicit OAuth2Service(const QString& auth_url,
                           const QString& token_url,
                           const QString& client_id,
                           const QString& client_secret,
                           const QString& scope,
                           QObject* parent = nullptr);
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
    void setRedirectUrl(const QString& redirect_url, bool start_handler);

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

    // Super secret fallback client ID/SECRET.
    QString clientSecretId() const;
    void setClientSecretId(const QString& client_secret_id);

    QString clientSecretSecret() const;
    void setClientSecretSecret(const QString& client_secret_secret);

    bool useHttpBasicAuthWithClientData() const;
    void setUseHttpBasicAuthWithClientData(bool use_auth);

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
    bool login(const std::function<void()>& functor_when_logged_in = std::function<void()>());

    // Removes all state data and stops redirection handler.
    void logout(bool stop_redirection_handler = true);

  protected:
    virtual void timerEvent(QTimerEvent* event);

  private slots:
    void startRefreshTimer();
    void killRefreshTimer();
    void tokenRequestFinished(QNetworkReply* network_reply);

  private:
    QString properClientId() const;
    QString properClientSecret() const;

  private:
    QString m_id;
    int m_timerId;
    QDateTime m_tokensExpireIn;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_tokenGrantType;
    QString m_clientId;
    QString m_clientSecret;
    QString m_clientSecretId;
    QString m_clientSecretSecret;
    QUrl m_tokenUrl;
    QString m_authUrl;
    QString m_scope;
    bool m_useHttpBasicAuthWithClientData;
    SilentNetworkAccessManager m_networkManager;
    OAuthHttpHandler* m_redirectionHandler;
    std::function<void()> m_functorOnLogin;
};

#endif // OAUTH2SERVICE_H
