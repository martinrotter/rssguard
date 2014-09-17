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

#include "core/feedsmodelrootitem.h"

#include "miscellaneous/application.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelfeed.h"

#include <QVariant>


FeedsModelRootItem::FeedsModelRootItem(FeedsModelRootItem *parent_item)
  : m_kind(FeedsModelRootItem::RootItem),
    m_id(NO_PARENT_CATEGORY),
    m_title(QString()),
    m_description(QString()),
    m_icon(QIcon()),
    m_creationDate(QDateTime()),
    m_childItems(QList<FeedsModelRootItem*>()),
    m_parentItem(parent_item) {
  setupFonts();
}

FeedsModelRootItem::~FeedsModelRootItem() {
  qDeleteAll(m_childItems);
}

void FeedsModelRootItem::setupFonts() {
  m_normalFont = Application::font("FeedsView");
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
}

int FeedsModelRootItem::row() const {
  if (m_parentItem) {
    return m_parentItem->m_childItems.indexOf(const_cast<FeedsModelRootItem*>(this));
  }
  else {
    // This item has no parent. Therefore, its row index is 0.
    return 0;
  }
}

QVariant FeedsModelRootItem::data(int column, int role) const {
  Q_UNUSED(column)
  Q_UNUSED(role)

  // Do not return anything for the root item.
  return QVariant();
}

int FeedsModelRootItem::countOfAllMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfAllMessages();
  }

  return total_count;
}

bool FeedsModelRootItem::removeChild(FeedsModelRootItem *child) {
  return m_childItems.removeOne(child);
}

FeedsModelRootItem *FeedsModelRootItem::child(FeedsModelRootItem::Kind kind_of_child, const QString &identifier) {
  foreach (FeedsModelRootItem *child, childItems()) {
    if (child->kind() == kind_of_child) {
      if ((kind_of_child == Category && static_cast<FeedsModelCategory*>(child)->title() == identifier) ||
          (kind_of_child == Feed && static_cast<FeedsModelFeed*>(child)->url() == identifier)) {
        return child;
      }
    }
  }

  return NULL;
}

int FeedsModelRootItem::countOfUnreadMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfUnreadMessages();
  }

  return total_count;
}

bool FeedsModelRootItem::removeChild(int index) {
  if (index >= 0 && index < m_childItems.size()) {
    m_childItems.removeAt(index);
    return true;
  }
  else {
    return false;
  }
}

bool FeedsModelRootItem::isEqual(FeedsModelRootItem *lhs, FeedsModelRootItem *rhs) {
  return (lhs->kind() == rhs->kind()) && (lhs->id() == rhs->id());
}

bool FeedsModelRootItem::lessThan(FeedsModelRootItem *lhs, FeedsModelRootItem *rhs) {
  if (lhs->kind() == rhs->kind()) {
    return lhs->id() < rhs->id();
  }
  else {
    return false;
  }
}
