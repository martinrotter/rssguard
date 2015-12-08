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


class RootItem;

class TtRssResponse {
  public:
    explicit TtRssResponse(const QString &raw_content = QString());
    virtual ~TtRssResponse();

    bool isLoaded() const;

    int seq() const;
    int status() const;
    QString error() const;
    bool hasError() const;
    bool isNotLoggedIn() const;

  protected:
    QtJson::JsonObject m_rawContent;
};

class TtRssLoginResponse : public TtRssResponse {
  public:
    explicit TtRssLoginResponse(const QString &raw_content = QString());
    virtual ~TtRssLoginResponse();

    int apiLevel() const;
    QString sessionId() const;
};

class TtRssGetFeedsCategoriesResponse : public TtRssResponse {
  public:
    explicit TtRssGetFeedsCategoriesResponse(const QString &raw_content = QString());
    virtual ~TtRssGetFeedsCategoriesResponse();

    // Returns tree of feeds/categories.
    // Top-level root of the tree is not needed here.
    // Returned items do not have primary IDs assigned.
    RootItem *feedsCategories(bool obtain_icons, QString base_address = QString());
};

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

    // Logs user in.
    TtRssLoginResponse login(QNetworkReply::NetworkError &error);

    // Logs user out.
    TtRssResponse logout(QNetworkReply::NetworkError &error);

    // Gets feeds from the server.
    TtRssGetFeedsCategoriesResponse getFeedsCategories(QNetworkReply::NetworkError &error);

  private:   
    QString m_url;
    QString m_username;
    QString m_password;
    QString m_sessionId;
};

#endif // TTRSSNETWORKFACTORY_H
