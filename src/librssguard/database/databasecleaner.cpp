// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasecleaner.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/thread.h"

#include <QDebug>

DatabaseCleaner::DatabaseCleaner(QObject* parent) : QObject(parent) {}

void DatabaseCleaner::purgeDatabaseData(CleanerOrders which_data) {
  qDebugNN << LOGSEC_DB << "Performing database cleanup in thread:" << QUOTE_W_SPACE_DOT(getThreadID());

  // Inform everyone about the start of the process.
  emit purgeStarted();

  const int difference = 99 / 12;
  int progress = 0;

  if (which_data.m_removeReadMessages) {
    progress += difference;

    emit purgeProgress(progress, tr("Removing read articles..."));

    // Remove read messages.
    purgeReadMessages();
    progress += difference;

    emit purgeProgress(progress, tr("Read articles purged..."));
  }

  if (which_data.m_removeRecycleBin) {
    progress += difference;

    emit purgeProgress(progress, tr("Purging recycle bin..."));

    // Remove read messages.
    purgeRecycleBin();
    progress += difference;

    emit purgeProgress(progress, tr("Recycle bin purged..."));
  }

  if (which_data.m_removeOldMessages) {
    progress += difference;

    emit purgeProgress(progress, tr("Removing old articles..."));

    // Remove old messages.
    purgeOldMessages(which_data.m_barrierForRemovingOldMessagesInDays);
    progress += difference;

    emit purgeProgress(progress, tr("Old articles purged..."));
  }

  if (which_data.m_removeStarredMessages) {
    progress += difference;

    emit purgeProgress(progress, tr("Removing starred articles..."));

    // Remove old messages.
    purgeStarredMessages();
    progress += difference;

    emit purgeProgress(progress, tr("Starred articles purged..."));
  }

  if (which_data.m_shrinkDatabase) {
    progress += difference;

    emit purgeProgress(progress, tr("Shrinking database file..."));

    // Call driver-specific vacuuming function.
    qApp->database()->driver()->vacuumDatabase();
    progress += difference;

    emit purgeProgress(progress, tr("Database file shrinked..."));
  }

  emit purgeFinished();
}

void DatabaseCleaner::purgeStarredMessages() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::purgeImportantMessages(db);
  });
}

void DatabaseCleaner::purgeReadMessages() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::purgeReadMessages(db);
  });
}

void DatabaseCleaner::purgeOldMessages(int days) {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::purgeOldMessages(db, days);
  });
}

void DatabaseCleaner::purgeRecycleBin() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::purgeRecycleBin(db);
  });
}
