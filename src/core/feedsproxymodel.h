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

#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include "feedsmodelrootitem.h"

#include <QSortFilterProxyModel>


class FeedsModel;

class FeedsProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsProxyModel(QObject *parent = 0);
    virtual ~FeedsProxyModel();

    // Access to the source model.
    inline FeedsModel *sourceModel() {
      return m_sourceModel;
    }

    QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const;

    // Maps list of indexes.
    QModelIndexList mapListToSource(const QModelIndexList &indexes);

    bool showUnreadOnly() const;
    void setShowUnreadOnly(bool show_unread_only);

    FeedsModelRootItem *selectedItem() const;
    void setSelectedItem(FeedsModelRootItem *selected_item);

  public slots:
    void invalidateFilter();

  protected:
    // Compares two rows of data.
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

  private:
    // Source model pointer.
    FeedsModel *m_sourceModel;

    FeedsModelRootItem *m_selectedItem;
    bool m_showUnreadOnly;
};

#endif // FEEDSPROXYMODEL_H
