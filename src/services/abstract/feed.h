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

#ifndef FEED_H
#define FEED_H

#include "core/rootitem.h"

#include "core/message.h"


// Base class for "feed" nodes.
class Feed : public RootItem {
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
      NetworkError  = 2,
      ParsingError  = 3,
      OtherError    = 4
    };

    // Constructors.
    explicit Feed(RootItem *parent = NULL);
    virtual ~Feed();

    // Returns 0, feeds have no children.
    int childCount() const;

    // Appending of childs to feed is not allowed.
    void appendChild(RootItem *child);

    // Performs synchronous update and returns number of newly updated messages.
    virtual int update() = 0;

    // Updates counts of all/unread messages for this feed.
    virtual void updateCounts(bool including_total_count) = 0;

    // Get ALL undeleted messages from this feed in one single list.
    virtual QList<Message> undeletedMessages() const = 0;

    inline int autoUpdateInitialInterval() const {
      return m_autoUpdateInitialInterval;
    }

    inline void setAutoUpdateInitialInterval(int auto_update_interval) {
      // If new initial auto-update interval is set, then
      // we should reset time that remains to the next auto-update.
      m_autoUpdateInitialInterval = auto_update_interval;
      m_autoUpdateRemainingInterval = auto_update_interval;
    }

    inline AutoUpdateType autoUpdateType() const {
      return m_autoUpdateType;
    }

    inline void setAutoUpdateType(const AutoUpdateType &autoUpdateType) {
      m_autoUpdateType = autoUpdateType;
    }

    inline int autoUpdateRemainingInterval() const {
      return m_autoUpdateRemainingInterval;
    }

    inline void setAutoUpdateRemainingInterval(int autoUpdateRemainingInterval) {
      m_autoUpdateRemainingInterval = autoUpdateRemainingInterval;
    }

    inline Status status() const {
      return m_status;
    }

    inline void setStatus(const Status &status) {
      m_status = status;
    }

  private:
    Status m_status;
    AutoUpdateType m_autoUpdateType;
    int m_autoUpdateInitialInterval;
    int m_autoUpdateRemainingInterval;
};

#endif // FEED_H
