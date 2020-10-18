// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/databasecleaner.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"

#include <QDebug>
#include <QThread>

DatabaseCleaner::DatabaseCleaner(QObject* parent) : QObject(parent) {}

void DatabaseCleaner::purgeDatabaseData(const CleanerOrders& which_data) {
  qDebugNN << LOGSEC_DB << "Performing database cleanup in thread: '" << QThread::currentThreadId() << "'.";

  // Inform everyone about the start of the process.
  emit purgeStarted();
  bool result = true;
  const int difference = 99 / 12;
  int progress = 0;
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

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

  if (which_data.m_removeStarredMessages) {
    progress += difference;
    emit purgeProgress(progress, tr("Removing starred messages..."));

    // Remove old messages.
    result &= purgeStarredMessages(database);
    progress += difference;
    emit purgeProgress(progress, tr("Starred messages purged..."));
  }

  result &= DatabaseQueries::purgeLeftoverLabelAssignments(database);

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
