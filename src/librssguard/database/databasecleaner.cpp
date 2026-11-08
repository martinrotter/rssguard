// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasecleaner.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/thread.h"

#include <QDataStream>
#include <QDebug>
#include <QHash>
#include <QScopeGuard>

DatabaseCleaner::DatabaseCleaner(QObject* parent) : QObject(parent) {}

CleanerOrders DatabaseCleaner::loadSettings() {
  auto* settings = qApp->settings();
  // Preserve choices saved by older versions until the dialog writes the explicit settings.
  QByteArray state = settings->value(GROUP(GUI), QSL("form_db_cleanup"), QByteArray()).toByteArray();
  QHash<QString, QHash<QString, QVariant>> legacy;
  QDataStream stream(&state, QIODevice::ReadOnly);
  stream >> legacy;

  if (stream.status() != QDataStream::Ok) {
    legacy.clear();
  }

  const auto value = [&](const QString& key, const QString& widget, const QString& property, const QVariant& fallback) {
    return settings->value(GROUP(Database), key, legacy.value(widget).value(property, fallback));
  };
  CleanerOrders orders;
  const QString checked = QSL("checked");

  orders.m_removeReadMessages = value(Database::CleanupRemoveReadMessages,
                                     QSL("m_checkRemoveReadMessages"),
                                     checked,
                                     orders.m_removeReadMessages)
                                 .toBool();
  orders.m_shrinkDatabase =
    value(Database::CleanupShrinkDatabase, QSL("m_checkShrink"), checked, orders.m_shrinkDatabase).toBool();
  orders.m_removeOldMessages = value(Database::CleanupRemoveOldMessages,
                                    QSL("m_checkRemoveOldMessages"),
                                    checked,
                                    orders.m_removeOldMessages)
                                .toBool();
  orders.m_removeRecycleBin = value(Database::CleanupRemoveRecycleBin,
                                   QSL("m_checkRemoveRecycleBin"),
                                   checked,
                                   orders.m_removeRecycleBin)
                               .toBool();
  orders.m_removeStarredMessages = value(Database::CleanupRemoveStarredMessages,
                                        QSL("m_checkRemoveStarredMessages"),
                                        checked,
                                        orders.m_removeStarredMessages)
                                    .toBool();
  const int days = value(Database::CleanupOlderThanDays,
                         QSL("m_spinDays"),
                         QSL("value"),
                         orders.m_barrierForRemovingOldMessagesInDays)
                     .toInt();

  orders.m_barrierForRemovingOldMessagesInDays = qBound(0, days, 1000); // Match the dialog, including zero days.
  return orders;
}

DatabaseCleaner::Timing DatabaseCleaner::automaticCleanupTiming() {
  const int timing = qApp->settings()->value(GROUP(Database), Database::CleanupTiming, int(Timing::Never)).toInt();
  return timing >= int(Timing::Never) && timing <= int(Timing::OnExit) ? Timing(timing) : Timing::Never;
}

void DatabaseCleaner::saveSettings(const CleanerOrders& orders, Timing timing) {
  auto* settings = qApp->settings();

  settings->setValue(GROUP(Database), Database::CleanupTiming, int(timing));
  settings->setValue(GROUP(Database), Database::CleanupRemoveReadMessages, orders.m_removeReadMessages);
  settings->setValue(GROUP(Database), Database::CleanupShrinkDatabase, orders.m_shrinkDatabase);
  settings->setValue(GROUP(Database), Database::CleanupRemoveOldMessages, orders.m_removeOldMessages);
  settings->setValue(GROUP(Database), Database::CleanupRemoveRecycleBin, orders.m_removeRecycleBin);
  settings->setValue(GROUP(Database), Database::CleanupRemoveStarredMessages, orders.m_removeStarredMessages);
  settings->setValue(GROUP(Database), Database::CleanupOlderThanDays, orders.m_barrierForRemovingOldMessagesInDays);
}

void DatabaseCleaner::runAutomatically(Timing timing) {
  if (timing == Timing::Never || automaticCleanupTiming() != timing) {
    return;
  }

  if (!qApp->feedUpdateLock()->tryLock()) {
    qWarningNN << LOGSEC_DB << "Skipping automatic database cleanup because another critical action is running.";
    return;
  }

  const auto unlock = qScopeGuard([]() { qApp->feedUpdateLock()->unlock(); });
  DatabaseCleaner cleaner;
  cleaner.purgeDatabaseData(loadSettings());
}

void DatabaseCleaner::purgeDatabaseData(CleanerOrders which_data) {
  qDebugNN << LOGSEC_DB << "Performing database cleanup in thread:" << QUOTE_W_SPACE_DOT(getThreadID());

  // Inform everyone about the start of the process.
  emit purgeStarted();

  try {
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

      emit purgeProgress(progress, tr("Removing important articles..."));

      // Remove old messages.
      purgeStarredMessages();
      progress += difference;

      emit purgeProgress(progress, tr("Important articles purged..."));
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
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Database cleanup failed:" << QUOTE_W_SPACE_DOT(ex.message());
    emit purgeFailed(ex.message());
  }
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
