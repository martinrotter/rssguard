// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef TTRSSNETWORKFACTORY_H
#define TTRSSNETWORKFACTORY_H

#include "qt-json/json.h"

#include <QString>
#include <QPair>
#include <QNetworkReply>


class TtRssResponse {
  public:
    explicit TtRssResponse(const QString &raw_content = QString());
    virtual ~TtRssResponse();

    bool isLoaded() const;

    int seq() const;
    int status() const;

  protected:
    QtJson::JsonObject m_rawContent;
};

class TtRssLoginResponse : public TtRssResponse {
  public:
    explicit TtRssLoginResponse(const QString &raw_content = QString());
    virtual ~TtRssLoginResponse();

    int apiLevel() const;
    QString sessionId() const;
    QString error() const;
    bool hasError() const;
};

typedef QPair<QNetworkReply::NetworkError,TtRssLoginResponse> LoginResult;

class TtRssNetworkFactory {
  public:
    explicit TtRssNetworkFactory();
    virtual ~TtRssNetworkFactory();

    QString url() const;
    void setUrl(const QString &url);

    QString username() const;
    void setUsername(const QString &username);

    QString password() const;
    void setPassword(const QString &password);

    // Operations.
    LoginResult login();

  private:
    QString m_url;
    QString m_username;
    QString m_password;
};

#endif // TTRSSNETWORKFACTORY_H
