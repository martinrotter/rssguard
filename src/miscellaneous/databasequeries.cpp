// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "miscellaneous/databasequeries.h"

#include <QVariant>


bool DatabaseQueries::markMessageReadUnread(QSqlDatabase db, int id, RootItem::ReadStatus read) {
  QSqlQuery query_read_msg(db);
  query_read_msg.setForwardOnly(true);

  if (!query_read_msg.prepare(QSL("UPDATE Messages SET is_read = :read WHERE id = :id;"))) {
    qWarning("Query preparation failed for message read change.");
    return false;
  }

  query_read_msg.bindValue(QSL(":id"), id);
  query_read_msg.bindValue(QSL(":read"), (int) read);

  return query_read_msg.exec();
}

DatabaseQueries::DatabaseQueries() {
}
