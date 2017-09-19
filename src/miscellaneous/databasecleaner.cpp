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

#include "miscellaneous/databasecleaner.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"

#include <QDebug>
#include <QThread>

DatabaseCleaner::DatabaseCleaner(QObject* parent) : QObject(parent) {}

DatabaseCleaner::~DatabaseCleaner() {}

void DatabaseCleaner::purgeDatabaseData(const CleanerOrders& which_data) {
  qDebug().nospace() << "Performing database cleanup in thread: \'" << QThread::currentThreadId() << "\'.";

  // Inform everyone about the start of the process.
  emit purgeStarted();
  bool result = true;
  const int difference = 99 / 8;
  int progress = 0;
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

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

bool DatabaseCleaner::purgeStarredMessages(const QSqlDatabase& database) {
  return DatabaseQueries::purgeImportantMessages(database);
}

bool DatabaseCleaner::purgeReadMessages(const QSqlDatabase& database) {
  return DatabaseQueries::purgeReadMessages(database);
}

bool DatabaseCleaner::purgeOldMessages(const QSqlDatabase& database, int days) {
  return DatabaseQueries::purgeOldMessages(database, days);
}

bool DatabaseCleaner::purgeRecycleBin(const QSqlDatabase& database) {
  return DatabaseQueries::purgeRecycleBin(database);
}
