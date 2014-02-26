// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FEEDSMODELSTANDARDCATEGORY_H
#define FEEDSMODELSTANDARDCATEGORY_H

#include "core/feedsmodelcategory.h"

#include <QSqlRecord>
#include <QDateTime>


// Represents STANDARD category container.
// Standard category container can contain:
//  a) other standard category containers,
//  b) standard feeds,
//  c) other containers and feeds (synchronized ones).
class FeedsModelStandardCategory : public FeedsModelCategory {
  public:
    // Constructors and destructors.
    explicit FeedsModelStandardCategory(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelStandardCategory();

    // Returns the actual data representation of standard category.
    QVariant data(int column, int role) const;

    // Removes category and all its children from persistent
    // database.
    bool removeItself();

    // Loads particular "standard category" from given sql record.
    static FeedsModelStandardCategory *loadFromRecord(const QSqlRecord &record);
};

#endif // FEEDSMODELSTANDARDCATEGORY_H
