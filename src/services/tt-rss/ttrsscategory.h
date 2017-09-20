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

#ifndef TTRSSCATEGORY_H
#define TTRSSCATEGORY_H

#include "services/abstract/category.h"

#include <QSqlRecord>

class TtRssServiceRoot;

class TtRssCategory : public Category {
  Q_OBJECT

  public:
    explicit TtRssCategory(RootItem* parent = nullptr);
    explicit TtRssCategory(const QSqlRecord& record);
    virtual ~TtRssCategory();

    TtRssServiceRoot* serviceRoot() const;

    bool markAsReadUnread(ReadStatus status);
};

#endif // TTRSSCATEGORY_H
