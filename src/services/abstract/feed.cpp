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

#include "services/abstract/feed.h"

#include "definitions/definitions.h"


Feed::Feed(RootItem *parent) : RootItem(parent) {
  m_status = Normal;
  m_autoUpdateType = DefaultAutoUpdate;
  m_autoUpdateInitialInterval = DEFAULT_AUTO_UPDATE_INTERVAL;
  m_autoUpdateRemainingInterval = DEFAULT_AUTO_UPDATE_INTERVAL;

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

        case NetworkError:
          return QColor(Qt::red);

        default:
          return QVariant();
      }

    default:
      return RootItem::data(column, role);
  }
}
