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

#ifndef OAUTH2SERVICE_H
#define OAUTH2SERVICE_H

#include <QObject>

#include "network-web/silentnetworkaccessmanager.h"

class OAuth2Service : public QObject {
  Q_OBJECT

  public:
    explicit OAuth2Service(QString authUrl, QString tokenUrl, QString clientId,
                           QString clientSecret, QString scope, QObject* parent = 0);

    void attachBearerHeader(QNetworkRequest& req);

    void setOAuthTokenGrantType(QString grant_type);
    QString grant_type();

    QString accessToken() const;
    void setAccessToken(const QString& access_token);

    QString refreshToken() const;
    void setRefreshToken(const QString& refresh_token);

  signals:
    void tokensReceived(QString access_token, QString refresh_token, int expires_in);
    void tokensRetrieveError(QString error, QString error_description);

    // User failed to authenticate or rejected it.
    void authFailed();

    // User enabled access.
    void authCodeObtained(QString auth_code);

  public slots:
    void retrieveAuthCode();
    void retrieveAccessToken(QString auth_code);
    void refreshAccessToken(QString refresh_token = QString());

    // Performs login if needed. If some refresh token is set, then
    // the initial "auth" step is skipped and attempt to refresh
    // access token is made.
    void login();

  private slots:
    void cleanTokens();
    void tokenRequestFinished(QNetworkReply* networkReply);

  private:
    QDateTime m_tokensExpireIn;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_redirectUri;
    QString m_tokenGrantType;
    QString m_clientId;
    QString m_clientSecret;
    QUrl m_tokenUrl;
    QString m_authUrl;
    QString m_scope;
    SilentNetworkAccessManager m_networkManager;
};

#endif // OAUTH2SERVICE_H
