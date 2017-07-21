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

#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include <QSortFilterProxyModel>


class FeedsModel;
class RootItem;

class FeedsProxyModel : public QSortFilterProxyModel {
		Q_OBJECT

	public:
		// Constructors and destructors.
		explicit FeedsProxyModel(FeedsModel* source_model, QObject* parent = 0);
		virtual ~FeedsProxyModel();

		// Returns index list of items which "match" given value.
		// Used for finding items according to entered title text.
		QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const;

		// Maps list of indexes.
		QModelIndexList mapListToSource(const QModelIndexList& indexes) const;

		bool showUnreadOnly() const;
		void setShowUnreadOnly(bool show_unread_only);

		const RootItem* selectedItem() const;
		void setSelectedItem(const RootItem* selected_item);

	public slots:
		void invalidateReadFeedsFilter(bool set_new_value = false, bool show_unread_only = false);

	private slots:
		void invalidateFilter();

	signals:
		void expandAfterFilterIn(QModelIndex idx) const;

	private:
		// Compares two rows of data.
		bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
		bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
		bool filterAcceptsRowInternal(int source_row, const QModelIndex& source_parent) const;

		// Source model pointer.
		FeedsModel* m_sourceModel;
		const RootItem* m_selectedItem;
		bool m_showUnreadOnly;
		QList<QPair<int, QModelIndex>> m_hiddenIndices;
};

#endif // FEEDSPROXYMODEL_H
