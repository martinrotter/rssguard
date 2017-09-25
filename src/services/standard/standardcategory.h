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

#ifndef FEEDSMODELCATEGORY_H
#define FEEDSMODELCATEGORY_H

#include "services/abstract/category.h"

#include <QCoreApplication>
#include <QSqlRecord>

class FeedsModel;
class StandardServiceRoot;

// Base class for all categories contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR category types.
// NOTE: This class should not be instantiated directly.
class StandardCategory : public Category {
  Q_OBJECT

  public:
    explicit StandardCategory(RootItem* parent_item = nullptr);
    explicit StandardCategory(const StandardCategory& other);
    explicit StandardCategory(const QSqlRecord& record);
    virtual ~StandardCategory();

    StandardServiceRoot* serviceRoot() const;

    // Returns the actual data representation of standard category.
    Qt::ItemFlags additionalFlags() const;
    bool performDragDropChange(RootItem* target_item);

    bool canBeEdited() const;
    bool canBeDeleted() const;

    bool editViaGui();
    bool deleteViaGui();

    bool addItself(RootItem* parent);
    bool editItself(StandardCategory* new_category_data);
    bool removeItself();
};

#endif // FEEDSMODELCLASSICCATEGORY_H
