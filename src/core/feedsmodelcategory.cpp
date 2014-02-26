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

#include "core/feedsmodelcategory.h"

#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"


FeedsModelCategory::FeedsModelCategory(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item) {
  m_kind = FeedsModelRootItem::Category;
}

FeedsModelCategory::FeedsModelCategory(const FeedsModelCategory &other)
  : FeedsModelRootItem(NULL) {
  m_kind = other.kind();
  m_title = other.title();
  m_id = other.id();
  m_icon = other.icon();
  m_childItems = other.childItems();
  m_parentItem = other.parent();
  m_type = other.type();
  m_creationDate = other.creationDate();
  m_description = other.description();
}

FeedsModelCategory::~FeedsModelCategory() {
}
