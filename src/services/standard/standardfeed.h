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

#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include "services/abstract/feed.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QMetaType>
#include <QNetworkReply>
#include <QPair>
#include <QSqlRecord>

class StandardServiceRoot;

// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class StandardFeed : public Feed {
  Q_OBJECT

  public:
    enum Type {
      Rss0X = 0,
      Rss2X = 1,
      Rdf = 2,      // Sometimes denoted as RSS 1.0.
      Atom10 = 3
    };

    // Constructors and destructors.
    explicit StandardFeed(RootItem* parent_item = nullptr);
    explicit StandardFeed(const StandardFeed& other);
    explicit StandardFeed(const QSqlRecord& record);
    virtual ~StandardFeed();

    StandardServiceRoot* serviceRoot() const;

    QList<QAction*> contextMenu();

    QString additionalTooltip() const;

    bool canBeEdited() const;
    bool canBeDeleted() const;

    bool editViaGui();
    bool deleteViaGui();

    // Obtains data related to this feed.
    Qt::ItemFlags additionalFlags() const;
    bool performDragDropChange(RootItem* target_item);

    bool addItself(RootItem* parent);
    bool editItself(StandardFeed* new_feed_data);
    bool removeItself();

    // Other getters/setters.
    Type type() const;
    void setType(Type type);
    bool passwordProtected() const;
    void setPasswordProtected(bool passwordProtected);
    QString username() const;
    void setUsername(const QString& username);
    QString password() const;
    void setPassword(const QString& password);
    QString encoding() const;
    void setEncoding(const QString& encoding);

    QNetworkReply::NetworkError networkError() const;

    // Tries to guess feed hidden under given URL
    // and uses given credentials.
    // Returns pointer to guessed feed (if at least partially
    // guessed) and retrieved error/status code from network layer
    // or NULL feed.
    static QPair<StandardFeed*, QNetworkReply::NetworkError> guessFeed(const QString& url,
                                                                       const QString& username = QString(),
                                                                       const QString& password = QString());

    // Converts particular feed type to string.
    static QString typeToString(Type type);

  public slots:
    void fetchMetadataForItself();

  private:
    QList<Message> obtainNewMessages(bool* error_during_obtaining);

  private:
    bool m_passwordProtected;
    QString m_username;
    QString m_password;
    Type m_type;

    QNetworkReply::NetworkError m_networkError;
    QString m_encoding;
};

Q_DECLARE_METATYPE(StandardFeed::Type)

#endif // FEEDSMODELFEED_H
