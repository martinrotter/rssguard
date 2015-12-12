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

#ifndef TTRSSFEED_H
#define TTRSSFEED_H

#include "services/abstract/feed.h"

#include <QSqlRecord>


class TtRssServiceRoot;

class TtRssFeed : public Feed {
    Q_OBJECT

  public:
    explicit TtRssFeed(RootItem *parent = NULL);
    explicit TtRssFeed(const QSqlRecord &record);
    virtual ~TtRssFeed();

    TtRssServiceRoot *serviceRoot();

    void updateCounts(bool including_total_count);

    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    int update();
    QList<Message> undeletedMessages() const;

    bool markAsReadUnread(ReadStatus status);

    int customId() const;
    void setCustomId(int custom_id);

  private:
    int updateMessages(const QList<Message> &messages);

    int m_customId;
    int m_totalCount;
    int m_unreadCount;
};

#endif // TTRSSFEED_H
