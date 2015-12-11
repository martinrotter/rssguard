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

#ifndef FEEDIMPORTEXPORTMODEL_H
#define FEEDIMPORTEXPORTMODEL_H

#include <QAbstractItemModel>

#include "services/abstract/rootitem.h"


class FeedsImportExportModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    enum Mode {
      Import,
      Export
    };

    // Constructors and destructors.
    explicit FeedsImportExportModel(QObject *parent = 0);
    virtual ~FeedsImportExportModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool isItemChecked(RootItem *item);

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    RootItem *itemForIndex(const QModelIndex &index) const;

    // Returns source QModelIndex on which lies given item.
    QModelIndex indexForItem(RootItem *item) const;

    // Root item manipulators.
    RootItem *rootItem() const;
    void setRootItem(RootItem *rootItem);

    // Exports to OPML 2.0
    // NOTE: http://dev.opml.org/spec2.html
    bool exportToOMPL20(QByteArray &result);
    bool importAsOPML20(const QByteArray &data);

    Mode mode() const;
    void setMode(const Mode &mode);

  public slots:
    void checkAllItems();
    void uncheckAllItems();

  private:
    QHash<RootItem*, Qt::CheckState> m_checkStates;
    RootItem *m_rootItem;

    bool m_recursiveChange;
    Mode m_mode;
};

#endif // FEEDIMPORTEXPORTMODEL_H
