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
#include <QThread>


DatabaseCleaner::DatabaseCleaner(QObject *parent) : QObject(parent) {
}

DatabaseCleaner::~DatabaseCleaner() {
}

void DatabaseCleaner::purgeDatabaseData(const CleanerOrders &which_data) {
  qDebug().nospace() << "Performing database cleanup in thread: \'" << QThread::currentThreadId() << "\'.";

  bool result = true;
  int progress = 0;

  emit purgeStarted();

  if (which_data.m_shrinkDatabase) {
    progress += 25;
    emit purgeProgress(progress, tr("Shrinking database file..."));

    result &= qApp->database()->vacuumDatabase();

    progress += 25;
    emit purgeProgress(progress, tr("Database file shrinked..."));
  }

  emit purgeFinished(result);
}
