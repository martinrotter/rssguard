// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "gui/treewidget.h"

#include <QMouseEvent>

TreeWidget::TreeWidget(QWidget* parent)
  : QTreeWidget(parent), m_refreshAllItemsNeeded(true), m_showMode(ItemShowMode::ItemsCollapsed) {
  connect(this, &TreeWidget::itemChanged, this, &TreeWidget::sheduleRefresh);
}

void TreeWidget::clear() {
  QTreeWidget::clear();
  m_allTreeItems.clear();
}

void TreeWidget::sheduleRefresh() {
  m_refreshAllItemsNeeded = true;
}

void TreeWidget::addTopLevelItem(QTreeWidgetItem* item) {
  m_allTreeItems.append(item);
  QTreeWidget::addTopLevelItem(item);
}

void TreeWidget::addTopLevelItems(const QList<QTreeWidgetItem*>& items) {
  m_allTreeItems.append(items);
  QTreeWidget::addTopLevelItems(items);
}

void TreeWidget::insertTopLevelItem(int index, QTreeWidgetItem* item) {
  m_allTreeItems.append(item);
  QTreeWidget::insertTopLevelItem(index, item);
}

void TreeWidget::insertTopLevelItems(int index, const QList<QTreeWidgetItem*>& items) {
  m_allTreeItems.append(items);
  QTreeWidget::insertTopLevelItems(index, items);
}

void TreeWidget::mousePressEvent(QMouseEvent* event) {
  if (event->modifiers() == Qt::ControlModifier) {
    emit itemControlClicked(itemAt(event->pos()));
  }

  if (event->buttons() == Qt::MiddleButton) {
    emit itemMiddleButtonClicked(itemAt(event->pos()));
  }

  QTreeWidget::mousePressEvent(event);
}

void TreeWidget::iterateAllItems(QTreeWidgetItem* parent) {
  int count = parent ? parent->childCount() : topLevelItemCount();

  for (int i = 0; i < count; i++) {
    QTreeWidgetItem* item = parent ? parent->child(i) : topLevelItem(i);

    if (item->childCount() == 0) {
      m_allTreeItems.append(item);
    }

    iterateAllItems(item);
  }
}

QList<QTreeWidgetItem*> TreeWidget::allItems() {
  if (m_refreshAllItemsNeeded) {
    m_allTreeItems.clear();
    iterateAllItems(0);
    m_refreshAllItemsNeeded = false;
  }

  return m_allTreeItems;
}

void TreeWidget::filterString(const QString& string) {
  QList<QTreeWidgetItem*> _allItems = allItems();
  QList<QTreeWidgetItem*> parents;
  bool stringIsEmpty = string.isEmpty();

  for (QTreeWidgetItem* item : _allItems) {
    bool containsString = stringIsEmpty || item->text(0).contains(string, Qt::CaseInsensitive);

    if (containsString) {
      item->setHidden(false);

      if (item->parent()) {
        if (!parents.contains(item->parent())) {
          parents << item->parent();
        }
      }
    }
    else {
      item->setHidden(true);

      if (item->parent()) {
        item->parent()->setHidden(true);
      }
    }
  }

  for (int i = 0; i < parents.size(); ++i) {
    QTreeWidgetItem* parentItem = parents.at(i);

    parentItem->setHidden(false);

    if (stringIsEmpty) {
      parentItem->setExpanded(m_showMode == ItemShowMode::ItemsExpanded);
    }
    else {
      parentItem->setExpanded(true);
    }

    if (parentItem->parent() && !parents.contains(parentItem->parent())) {
      parents << parentItem->parent();
    }
  }
}

bool TreeWidget::appendToParentItem(const QString& parentText, QTreeWidgetItem* item) {
  QList<QTreeWidgetItem*> list = findItems(parentText, Qt::MatchExactly);

  if (list.count() == 0) {
    return false;
  }

  QTreeWidgetItem* parentItem = list.at(0);

  if (!parentItem) {
    return false;
  }

  m_allTreeItems.append(item);
  parentItem->addChild(item);
  return true;
}

bool TreeWidget::appendToParentItem(QTreeWidgetItem* parent, QTreeWidgetItem* item) {
  if (!parent || parent->treeWidget() != this) {
    return false;
  }

  m_allTreeItems.append(item);
  parent->addChild(item);
  return true;
}

bool TreeWidget::prependToParentItem(const QString& parentText, QTreeWidgetItem* item) {
  QList<QTreeWidgetItem*> list = findItems(parentText, Qt::MatchExactly);

  if (list.count() == 0) {
    return false;
  }

  QTreeWidgetItem* parentItem = list.at(0);

  if (!parentItem) {
    return false;
  }

  m_allTreeItems.append(item);
  parentItem->insertChild(0, item);
  return true;
}

bool TreeWidget::prependToParentItem(QTreeWidgetItem* parent, QTreeWidgetItem* item) {
  if (!parent || parent->treeWidget() != this) {
    return false;
  }

  m_allTreeItems.append(item);
  parent->insertChild(0, item);
  return true;
}

void TreeWidget::deleteItem(QTreeWidgetItem* item) {
  m_refreshAllItemsNeeded = true;
  delete item;
}

void TreeWidget::deleteItems(const QList<QTreeWidgetItem*>& items) {
  m_refreshAllItemsNeeded = true;
  qDeleteAll(items);
}
