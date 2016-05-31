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

#ifndef FEED_H
#define FEED_H

#include "services/abstract/rootitem.h"

#include "core/message.h"

#include <QVariant>
#include <QRunnable>


// Base class for "feed" nodes.
class Feed : public RootItem, public QRunnable {
    Q_OBJECT

  public:
    // Specifies the auto-update strategy for the feed.
    enum AutoUpdateType {
      DontAutoUpdate      = 0,
      DefaultAutoUpdate   = 1,
      SpecificAutoUpdate  = 2
    };

    // Specifies the actual "status" of the feed.
    // For example if it has new messages, error
    // occurred, and so on.
    enum Status {
      Normal        = 0,
      NewMessages   = 1,
      Error         = 2,
      ParsingError  = 3,
      OtherError    = 4
    };

    // Constructors.
    explicit Feed(RootItem *parent = NULL);
    virtual ~Feed();

    QList<Message> undeletedMessages() const;

    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    void setCountOfAllMessages(int count_all_messages);
    void setCountOfUnreadMessages(int count_unread_messages);

    // Performs synchronous update and returns number of newly updated messages.
    // NOTE: This is called from worker thread, not from main UI thread.
    // NOTE: This should COMPLETELY download ALL messages from online source
    // into locale "Messages" table, INCLUDING contents (or excerpts) of those
    // messages.
    int update();

    QVariant data(int column, int role) const;

    int autoUpdateInitialInterval() const;
    void setAutoUpdateInitialInterval(int auto_update_interval);

    AutoUpdateType autoUpdateType() const;
    void setAutoUpdateType(AutoUpdateType auto_update_type);

    int autoUpdateRemainingInterval() const;
    void setAutoUpdateRemainingInterval(int auto_update_remaining_interval);

    inline Status status() const {
      return m_status;
    }

    inline void setStatus(const Status &status) {
      m_status = status;
    }

    inline QString url() const {
      return m_url;
    }

    inline void setUrl(const QString &url) {
      m_url = url;
    }

    void updateCounts(bool including_total_count);

    // Runs update in thread (thread pooled).
    void run();

  protected:
    virtual QList<Message> obtainNewMessages() = 0;

  private:
    int updateMessages(const QList<Message> &messages);

  signals:
    void updated(int updated_messages);

  private:
    QString m_url;
    Status m_status;
    AutoUpdateType m_autoUpdateType;
    int m_autoUpdateInitialInterval;
    int m_autoUpdateRemainingInterval;
    int m_totalCount;
    int m_unreadCount;
};

Q_DECLARE_METATYPE(Feed::AutoUpdateType)

#endif // FEED_H
