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


#ifndef MESSAGESMODELSQLLAYER_H
#define MESSAGESMODELSQLLAYER_H

#include <QSqlDatabase>

#include <QMap>
#include <QList>


class MessagesModelSqlLayer {
	public:
		explicit MessagesModelSqlLayer();

		// Adds this new state to queue of sort states.
		void addSortState(int column, Qt::SortOrder order);

		// Sets SQL WHERE clause, without "WHERE" keyword.
		void setFilter(const QString& filter);

	protected:
		QString orderByClause() const;
		QString selectStatement() const;
		QString formatFields() const;

		QSqlDatabase m_db;

	private:
		QString m_filter;

		// NOTE: These two lists contain data for multicolumn sorting.
		// They are always same length. Most important sort column/order
		// are located at the start of lists;
		QMap<int, QString> m_fieldNames;
		QList<int> m_sortColumns;
		QList<Qt::SortOrder> m_sortOrders;
};

#endif // MESSAGESMODELSQLLAYER_H
