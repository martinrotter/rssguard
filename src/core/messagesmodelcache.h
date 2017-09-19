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

#ifndef MESSAGESMODELCACHE_H
#define MESSAGESMODELCACHE_H

#include <QObject>

#include "core/message.h"

#include <QModelIndex>
#include <QVariant>

class MessagesModelCache : public QObject {
  Q_OBJECT

  public:
    explicit MessagesModelCache(QObject* parent = nullptr);
    virtual ~MessagesModelCache();

    inline bool containsData(int row_idx) const {
      return m_msgCache.contains(row_idx);
    }

    inline QSqlRecord record(int row_idx) const {
      return m_msgCache.value(row_idx);
    }

    inline void clear() {
      m_msgCache.clear();
    }

    void setData(const QModelIndex& index, const QVariant& value, const QSqlRecord& record);

    QVariant data(const QModelIndex& idx);

  private:
    QHash<int, QSqlRecord> m_msgCache;
};

#endif // MESSAGESMODELCACHE_H
