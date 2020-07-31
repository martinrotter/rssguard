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

#ifndef BOOKMARKSTREEWIDGET_H
#define BOOKMARKSTREEWIDGET_H

#include <QTreeWidget>

class TreeWidget : public QTreeWidget {
  Q_OBJECT

  public:
    explicit TreeWidget(QWidget* parent = 0);

    enum class ItemShowMode {
      ItemsCollapsed = 0,
      ItemsExpanded = 1
    };

    ItemShowMode defaultItemShowMode() {
      return m_showMode;
    }

    void setDefaultItemShowMode(ItemShowMode mode) {
      m_showMode = mode;
    }

    QList<QTreeWidgetItem*> allItems();

    bool appendToParentItem(const QString& parentText, QTreeWidgetItem* item);
    bool appendToParentItem(QTreeWidgetItem* parent, QTreeWidgetItem* item);
    bool prependToParentItem(const QString& parentText, QTreeWidgetItem* item);
    bool prependToParentItem(QTreeWidgetItem* parent, QTreeWidgetItem* item);

    void addTopLevelItem(QTreeWidgetItem* item);
    void addTopLevelItems(const QList<QTreeWidgetItem*>& items);
    void insertTopLevelItem(int index, QTreeWidgetItem* item);
    void insertTopLevelItems(int index, const QList<QTreeWidgetItem*>& items);

    void deleteItem(QTreeWidgetItem* item);
    void deleteItems(const QList<QTreeWidgetItem*>& items);

  signals:
    void itemControlClicked(QTreeWidgetItem* item);
    void itemMiddleButtonClicked(QTreeWidgetItem* item);

  public slots:
    void filterString(const QString& string);
    void clear();

  private slots:
    void sheduleRefresh();

  private:
    void mousePressEvent(QMouseEvent* event);
    void iterateAllItems(QTreeWidgetItem* parent);

    bool m_refreshAllItemsNeeded;
    QList<QTreeWidgetItem*> m_allTreeItems;
    ItemShowMode m_showMode;
};

#endif // BOOKMARKSTREEWIDGET_H
