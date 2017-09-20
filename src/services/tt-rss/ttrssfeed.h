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

#ifndef TTRSSFEED_H
#define TTRSSFEED_H

#include "services/abstract/feed.h"

#include <QSqlRecord>

class TtRssServiceRoot;

class TtRssFeed : public Feed {
  Q_OBJECT

  public:
    explicit TtRssFeed(RootItem* parent = nullptr);
    explicit TtRssFeed(const QSqlRecord& record);
    virtual ~TtRssFeed();

    TtRssServiceRoot* serviceRoot() const;

    bool canBeEdited() const;
    bool editViaGui();
    bool canBeDeleted() const;
    bool deleteViaGui();

    bool markAsReadUnread(ReadStatus status);

    bool editItself(TtRssFeed* new_feed_data);
    bool removeItself();

  private:
    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // TTRSSFEED_H
