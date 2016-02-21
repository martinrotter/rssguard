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

#include "services/abstract/feed.h"

#include "definitions/definitions.h"


Feed::Feed(RootItem *parent)
  : RootItem(parent), m_url(QString()), m_status(Normal), m_autoUpdateType(DefaultAutoUpdate),
    m_autoUpdateInitialInterval(DEFAULT_AUTO_UPDATE_INTERVAL), m_autoUpdateRemainingInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_totalCount(0), m_unreadCount(0) {
  setKind(RootItemKind::Feed);
}

Feed::~Feed() {
}

QVariant Feed::data(int column, int role) const {
  switch (role) {
    case Qt::ForegroundRole:
      switch (status()) {
        case NewMessages:
          return QColor(Qt::blue);

        case Error:
          return QColor(Qt::red);

        default:
          return QVariant();
      }

    default:
      return RootItem::data(column, role);
  }
}

int Feed::autoUpdateInitialInterval() const {
  return m_autoUpdateInitialInterval;
}

int Feed::countOfAllMessages() const {
  return m_totalCount;
}

int Feed::countOfUnreadMessages() const {
  return m_unreadCount;
}

void Feed::setCountOfAllMessages(int count_all_messages) {
  m_totalCount = count_all_messages;
}

void Feed::setCountOfUnreadMessages(int count_unread_messages) {
  m_unreadCount = count_unread_messages;
}

void Feed::setAutoUpdateInitialInterval(int auto_update_interval) {
  // If new initial auto-update interval is set, then
  // we should reset time that remains to the next auto-update.
  m_autoUpdateInitialInterval = auto_update_interval;
  m_autoUpdateRemainingInterval = auto_update_interval;
}

Feed::AutoUpdateType Feed::autoUpdateType() const {
  return m_autoUpdateType;
}

void Feed::setAutoUpdateType(Feed::AutoUpdateType auto_update_type) {
  m_autoUpdateType = auto_update_type;
}

int Feed::autoUpdateRemainingInterval() const {
  return m_autoUpdateRemainingInterval;
}

void Feed::setAutoUpdateRemainingInterval(int auto_update_remaining_interval) {
  m_autoUpdateRemainingInterval = auto_update_remaining_interval;
}
