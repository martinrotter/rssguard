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


bool DatabaseQueries::markMessagesRead(QSqlDatabase db, const QStringList &ids, RootItem::ReadStatus read) {
  QSqlQuery query_read_msg(db);
  query_read_msg.setForwardOnly(true);

  return query_read_msg.exec(QString(QSL("UPDATE Messages SET is_read = %2 WHERE id IN (%1);"))
                             .arg(ids.join(QSL(", ")), read == RootItem::Read ? QSL("1") : QSL("0")));
}

bool DatabaseQueries::markMessageImportant(QSqlDatabase db, int id, RootItem::Importance importance) {
  QSqlQuery q;(db);
  q.setForwardOnly(true);

  if (!q.prepare(QSL("UPDATE Messages SET is_important = :important WHERE id = :id;"))) {
    qWarning("Query preparation failed for message importance switch.");
    return false;
  }

  q.bindValue(QSL(":id"), id);
  q.bindValue(QSL(":important"), (int) importance);

  // Commit changes.
  return q.exec();
}

bool DatabaseQueries::switchMessagesImportance(QSqlDatabase db, const QStringList &ids) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  return q.exec(QString(QSL("UPDATE Messages SET is_important = NOT is_important WHERE id IN (%1);")).arg(ids.join(QSL(", "))));
}

bool DatabaseQueries::permanentlyDeleteMessages(QSqlDatabase db, const QStringList &ids) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  return q.exec(QString(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE id IN (%1);")).arg(ids.join(QSL(", "))));
}

bool DatabaseQueries::deleteOrRestoreMessagesToFromBin(QSqlDatabase db, const QStringList &ids, bool deleted) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  return q.exec(QString(QSL("UPDATE Messages SET is_deleted = %2 WHERE id IN (%1);")).arg(ids.join(QSL(", ")),
                                                                                          QString::number(deleted ? 1 : 0)));
}

DatabaseQueries::DatabaseQueries() {
}
