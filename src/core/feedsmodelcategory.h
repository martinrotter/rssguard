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

#ifndef FEEDSMODELCATEGORY_H
#define FEEDSMODELCATEGORY_H

#include "core/feedsmodelrootitem.h"


class FeedsModelFeed;

// Base class for all categories contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR category types.
// NOTE: This class should not be instantiated directly.
class FeedsModelCategory : public FeedsModelRootItem {
  public:
    // Describes possible types of categories.
    // NOTE: This is equivavelnt to Categories(type).
    enum Type {
      Standard    = 0,
      Feedly      = 1
    };

    // Constructors and destructors
    explicit FeedsModelCategory(FeedsModelRootItem *parent_item = NULL);
    explicit FeedsModelCategory(const FeedsModelCategory &other);
    virtual ~FeedsModelCategory();

    // All types of categories offer these getters/setters.
    inline Type type() const {
      return m_type;
    }

    inline void setType(const Type &type) {
      m_type = type;
    }

  protected:
    Type m_type;
};

#endif // FEEDSMODELCLASSICCATEGORY_H
