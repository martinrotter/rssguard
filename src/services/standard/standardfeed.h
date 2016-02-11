// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include <QMetaType>
#include <QDateTime>
#include <QSqlRecord>
#include <QPair>
#include <QNetworkReply>
#include <QCoreApplication>


class Message;
class FeedsModel;
class StandardServiceRoot;

// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class StandardFeed : public Feed {
    Q_OBJECT

  public:
    // Describes possible types of feeds.
    // NOTE: This is equivalent to attribute Feeds(type).
    enum Type {
      Rss0X   = 0,
      Rss2X   = 1,
      Rdf     = 2,  // Sometimes denoted as RSS 1.0.
      Atom10  = 3
    };

    // Constructors and destructors.
    explicit StandardFeed(RootItem *parent_item = NULL);
    explicit StandardFeed(const StandardFeed &other);
    explicit StandardFeed(const QSqlRecord &record);
    virtual ~StandardFeed();

    StandardServiceRoot *serviceRoot() const;
    QList<QAction*> contextMenu();

    bool canBeEdited() const {
      return true;
    }

    bool canBeDeleted() const {
      return true;
    }

    bool editViaGui();
    bool deleteViaGui();

    bool markAsReadUnread(ReadStatus status);
    bool cleanMessages(bool clean_read_only);

    QList<Message> undeletedMessages() const;

    QVariant data(int column, int role) const;

    // Obtains data related to this feed.
    Qt::ItemFlags additionalFlags() const;
    bool performDragDropChange(RootItem *target_item);

    // Perform fetching of new messages. Returns number of newly updated messages.
    int update();

    // Updates counts of all/unread messages for this feed.
    void updateCounts(bool including_total_count);

    // Removes this standard feed from persistent
    // storage.
    bool removeItself();
    bool addItself(RootItem *parent);
    bool editItself(StandardFeed *new_feed_data);

    int messageForeignKeyId() const;

    // Other getters/setters.
    inline Type type() const {
      return m_type;
    }

    inline void setType(const Type &type) {
      m_type = type;
    }

    inline bool passwordProtected() const {
      return m_passwordProtected;
    }

    inline void setPasswordProtected(bool passwordProtected) {
      m_passwordProtected = passwordProtected;
    }

    inline QString username() const {
      return m_username;
    }

    inline void setUsername(const QString &username) {
      m_username = username;
    }

    inline QString password() const {
      return m_password;
    }

    inline void setPassword(const QString &password) {
      m_password = password;
    }

    inline QString encoding() const {
      return m_encoding;
    }

    inline void setEncoding(const QString &encoding) {
      m_encoding = encoding;
    }

    QNetworkReply::NetworkError networkError() const;

    // Tries to guess feed hidden under given URL
    // and uses given credentials.
    // Returns pointer to guessed feed (if at least partially
    // guessed) and retrieved error/status code from network layer
    // or NULL feed.
    static QPair<StandardFeed*,QNetworkReply::NetworkError> guessFeed(const QString &url,
                                                                      const QString &username = QString(),
                                                                      const QString &password = QString());

    // Converts particular feed type to string.
    static QString typeToString(Type type);

  public slots:
    // Fetches metadata for the feed.
    void fetchMetadataForItself();

  private:
    // Persistently stores given messages into the database
    // and updates existing messages if newer version is
    // available.
    int updateMessages(const QList<Message> &messages);

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
