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

#include "core/rootitem.h"

#include "services/abstract/serviceroot.h"
#include "services/abstract/feed.h"
#include "services/abstract/category.h"
#include "miscellaneous/application.h"

#include <QVariant>


RootItem::RootItem(RootItem *parent_item)
  : QObject(NULL),
    m_kind(RootItemKind::Root),
    m_id(NO_PARENT_CATEGORY),
    m_title(QString()),
    m_description(QString()),
    m_icon(QIcon()),
    m_creationDate(QDateTime()),
    m_childItems(QList<RootItem*>()),
    m_parentItem(parent_item) {
  setupFonts();
}

RootItem::~RootItem() {
  qDeleteAll(m_childItems);
}

QList<QAction*> RootItem::contextMenuActions() {
  return QList<QAction*>();
}

bool RootItem::canBeEdited() {
  return false;
}

bool RootItem::editViaGui() {
  return false;
}

bool RootItem::canBeDeleted() {
  return false;
}

bool RootItem::deleteViaGui() {
  return false;
}

bool RootItem::canBeMarkedAsReadUnread(ReadStatus status) {
  return true;
}

bool RootItem::markAsReadUnread(ReadStatus status) {
  bool result = true;

  foreach (RootItem *child, m_childItems) {
    if (child->canBeMarkedAsReadUnread(status)) {
      result &= child->markAsReadUnread(status);
    }
  }

  return result;
}

bool RootItem::cleanMessages(bool clear_only_read) {
  bool result = true;

  foreach (RootItem *child, m_childItems) {
    result &= child->cleanMessages(clear_only_read);
  }

  return result;
}

void RootItem::updateCounts(bool including_total_count) {
  foreach (RootItem *child, m_childItems) {
    child->updateCounts(including_total_count);
  }
}

void RootItem::setupFonts() {
  m_normalFont = Application::font("FeedsView");
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
}

int RootItem::row() const {
  if (m_parentItem) {
    return m_parentItem->m_childItems.indexOf(const_cast<RootItem*>(this));
  }
  else {
    // This item has no parent. Therefore, its row index is 0.
    return 0;
  }
}

QVariant RootItem::data(int column, int role) const {
  Q_UNUSED(column)
  Q_UNUSED(role)

  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread message(s).", 0, countOfUnreadMessages());
      }
      else {
        return QVariant();
      }

    case Qt::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        int count_all = countOfAllMessages();
        int count_unread = countOfUnreadMessages();

        return qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString()
            .replace(PLACEHOLDER_UNREAD_COUNTS, count_unread < 0 ? QSL("-") : QString::number(count_unread))
            .replace(PLACEHOLDER_ALL_COUNTS, count_all < 0 ? QSL("-") : QString::number(count_all));
      }
      else {
        return QVariant();
      }

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return icon();
      }
      else {
        return QVariant();
      }

    case Qt::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}

Qt::ItemFlags RootItem::additionalFlags() const {
  return Qt::NoItemFlags;
}

bool RootItem::performDragDropChange(RootItem *target_item) {
  return false;
}

int RootItem::countOfAllMessages() const {
  int total_count = 0;

  foreach (RootItem *child_item, m_childItems) {
    total_count += child_item->countOfAllMessages();
  }

  return total_count;
}

bool RootItem::isChildOf(RootItem *root) {
  if (root == NULL) {
    return false;
  }

  RootItem *this_item = this;

  while (this_item->kind() != RootItemKind::Root) {
    if (root->childItems().contains(this_item)) {
      return true;
    }
    else {
      this_item = this_item->parent();
    }
  }

  return false;
}

bool RootItem::isParentOf(RootItem *child) {
  if (child == NULL) {
    return false;
  }
  else {
    return child->isChildOf(this);
  }
}

QList<RootItem*> RootItem::getSubTree() {
  QList<RootItem*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(this);

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem *active_item = traversable_items.takeFirst();

    children.append(active_item);
    traversable_items.append(active_item->childItems());
  }

  return children;
}

QList<RootItem*> RootItem::getSubTree(RootItemKind::Kind kind_of_item) {
  QList<RootItem*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(this);

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem *active_item = traversable_items.takeFirst();

    if ((active_item->kind() & kind_of_item) > 0) {
      children.append(active_item);
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QList<Category*> RootItem::getSubTreeCategories() {
  QList<Category*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(this);

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem *active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItemKind::Category) {
      children.append(active_item->toCategory());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QList<Feed*> RootItem::getSubTreeFeeds() {
  QList<Feed*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(this);

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem *active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItemKind::Feed) {
      children.append(active_item->toFeed());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

ServiceRoot *RootItem::getParentServiceRoot() {
  RootItem *working_parent = this;

  while (working_parent->kind() != RootItemKind::Root) {
    if (working_parent->kind() == RootItemKind::ServiceRoot) {
      return working_parent->toServiceRoot();
    }
    else {
      working_parent = working_parent->parent();
    }
  }

  return NULL;
}

bool RootItem::removeChild(RootItem *child) {
  return m_childItems.removeOne(child);
}

Category *RootItem::toCategory() {
  return static_cast<Category*>(this);
}

Feed *RootItem::toFeed() {
  return static_cast<Feed*>(this);
}

ServiceRoot *RootItem::toServiceRoot() {
  return static_cast<ServiceRoot*>(this);
}

int RootItem::countOfUnreadMessages() const {
  int total_count = 0;

  foreach (RootItem *child_item, m_childItems) {
    total_count += child_item->countOfUnreadMessages();
  }

  return total_count;
}

bool RootItem::removeChild(int index) {
  if (index >= 0 && index < m_childItems.size()) {
    m_childItems.removeAt(index);
    return true;
  }
  else {
    return false;
  }
}
