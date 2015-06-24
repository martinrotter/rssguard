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

#include "miscellaneous/databasecleaner.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>


DatabaseCleaner::DatabaseCleaner(QObject *parent) : QObject(parent) {
  setObjectName("DatabaseCleaner");
}

DatabaseCleaner::~DatabaseCleaner() {
}

void DatabaseCleaner::purgeDatabaseData(const CleanerOrders &which_data) {
  qDebug().nospace() << "Performing database cleanup in thread: \'" << QThread::currentThreadId() << "\'.";

  // Inform everyone about the start of the process.
  emit purgeStarted();

  bool result = true;
  int difference = 99 / 8;
  int progress = 0;
  QSqlDatabase database = qApp->database()->connection(objectName(), DatabaseFactory::FromSettings);

  if (which_data.m_removeReadMessages) {
    progress += difference;
    emit purgeProgress(progress, tr("Removing read messages..."));

    // Remove read messages.
    result &= purgeReadMessages(database);

    progress += difference;
    emit purgeProgress(progress, tr("Read messages purged..."));
  }

  if (which_data.m_removeRecycleBin) {
    progress += difference;
    emit purgeProgress(progress, tr("Purging recycle bin..."));

    // Remove read messages.
    result &= purgeRecycleBin(database);

    progress += difference;
    emit purgeProgress(progress, tr("Recycle bin purged..."));
  }

  if (which_data.m_removeOldMessages) {
    progress += difference;
    emit purgeProgress(progress, tr("Removing old messages..."));

    // Remove old messages.
    result &= purgeOldMessages(database, which_data.m_barrierForRemovingOldMessagesInDays);

    progress += difference;
    emit purgeProgress(progress, tr("Old messages purged..."));
  }

  if (which_data.m_shrinkDatabase) {
    progress += difference;
    emit purgeProgress(progress, tr("Shrinking database file..."));

    // Call driver-specific vacuuming function.
    result &= qApp->database()->vacuumDatabase();

    progress += difference;
    emit purgeProgress(progress, tr("Database file shrinked..."));
  }

  emit purgeFinished(result);
}

bool DatabaseCleaner::purgeStarredMessages(const QSqlDatabase &database) {
  QSqlQuery query = QSqlQuery(database);

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important;"));
  query.bindValue(QSL(":is_important"), 1);

  return query.exec();
}

bool DatabaseCleaner::purgeReadMessages(const QSqlDatabase &database) {
  QSqlQuery query = QSqlQuery(database);

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND is_deleted = :is_deleted AND is_read = :is_read;"));
  query.bindValue(QSL(":is_read"), 1);

  // Remove only messages which are NOT in recycle bin.
  query.bindValue(QSL(":is_deleted"), 0);

  // Remove only messages which are NOT starred.
  query.bindValue(QSL(":is_important"), 0);

  return query.exec();
}

bool DatabaseCleaner::purgeOldMessages(const QSqlDatabase &database, int days) {
  QSqlQuery query = QSqlQuery(database);
  qint64 since_epoch = QDateTime::currentDateTimeUtc().addDays(-days).toMSecsSinceEpoch();

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND date_created < :date_created;"));
  query.bindValue(QSL(":date_created"), since_epoch);

  // Remove only messages which are NOT starred.
  query.bindValue(QSL(":is_important"), 0);

  return query.exec();
}

bool DatabaseCleaner::purgeRecycleBin(const QSqlDatabase &database) {
  QSqlQuery query = QSqlQuery(database);

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND is_deleted = :is_deleted;"));
  query.bindValue(QSL(":is_deleted"), 1);

  // Remove only messages which are NOT starred.
  query.bindValue(QSL(":is_important"), 0);

  return query.exec();
}
