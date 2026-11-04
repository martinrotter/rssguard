// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasedriver.h"

#include "database/databasefactory.h"
#include "database/sqlquery.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/sqlexception.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/thread.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QSqlError>
#include <QThread>

DatabaseDriver::DatabaseDriver(QObject* parent) : QObject(parent), m_databaseInitialized(false) {}

QSqlDatabase DatabaseDriver::threadSafeConnection(const QString& connection_name) {
  qlonglong thread_id = getThreadID();
  bool is_main_thread =
    QCoreApplication::instance() != nullptr && QThread::currentThread() == QCoreApplication::instance()->thread();
  QSqlDatabase database =
    connection(is_main_thread ? connection_name : QSL("%1_%2").arg(connection_name, QString::number(thread_id)));

  return database;
}

void DatabaseDriver::removeConnection(const QString& connection_name) {
  if (!QSqlDatabase::contains(connection_name)) {
    return;
  }

  QSqlDatabase database = QSqlDatabase::database(connection_name, false);

  database.close();
  database = QSqlDatabase();
  QSqlDatabase::removeDatabase(connection_name);
}

QString DatabaseDriver::limitOffset(int limit, int offset) const {
  if (limit > 0 && offset > 0) {
    return QSL("LIMIT %1 OFFSET %2").arg(QString::number(limit), QString::number(offset));
  }
  else if (limit > 0) {
    return QSL("LIMIT %1").arg(QString::number(limit));
  }
  else if (offset > 0) {
    // NOTE: This works for SQLite, but not for MariaDB, reimplemented in MariaDB driver.
    return QSL("LIMIT -1 OFFSET %1").arg(QString::number(offset));
  }
  else {
    return QString();
  }
}

QSqlDatabase DatabaseDriver::connection(const QString& connection_name) {
  QSqlDatabase database;

  if (!m_databaseInitialized) {
    finishRestoration();
    beforeAddDatabase();

    database = QSqlDatabase::addDatabase(qtDriverCode(), connection_name);

    afterAddDatabase(database, false);

    if (!database.isOpen() && !database.open()) {
      qFatal("Database was NOT opened. Delivered error message: %s", qPrintable(database.lastError().text()));
    }

    updateDatabaseSchema(database, databaseName());
    m_databaseInitialized = true;
  }
  else {
    if (QSqlDatabase::contains(connection_name)) {
      database = QSqlDatabase::database(connection_name);
    }
    else {
      database = QSqlDatabase::addDatabase(qtDriverCode(), connection_name);
      afterAddDatabase(database, true);
    }

    ensureConnectionUsable(database);
  }

  return database;
}

void DatabaseDriver::beforeAddDatabase() {}

void DatabaseDriver::afterAddDatabase(QSqlDatabase& database, bool was_initialized) {
  Q_UNUSED(database)
}

void DatabaseDriver::ensureConnectionUsable(QSqlDatabase& database) {
  if (database.isOpen()) {
    return;
  }

  if (!database.open()) {
    qFatal("Database was NOT opened. Delivered error message: %s.", qPrintable(database.lastError().text()));
  }

  SqlQuery query_db(database);
  setPragmas(query_db);
}

void DatabaseDriver::setPragmas(SqlQuery& query) {
  Q_UNUSED(query)
}

void DatabaseDriver::setForeignKeyChecksEnabled(const QSqlDatabase& db) {
  SqlQuery q(db);

  q.exec(foreignKeysEnable());
}

void DatabaseDriver::setForeignKeyChecksDisabled(const QSqlDatabase& db) {
  SqlQuery q(db);

  q.exec(foreignKeysDisable());
}

void DatabaseDriver::updateDatabaseSchema(QSqlDatabase& db, const QString& database_name) {
  SqlQuery query_db(db);
  setPragmas(query_db);

  // Sample query which checks for existence of tables.
  if (!query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"), false)) {
    qWarningNN << LOGSEC_DB << "Database is not initialized. Initializing now.";

    try {
      const QStringList statements = prepareScript(APP_SQL_PATH, QSL(APP_DB_INIT_FILE_PATTERN), database_name);

      for (const QString& statement : statements) {
        query_db.exec(statement);
      }

      setSchemaVersion(query_db, QSL(APP_DB_SCHEMA_VERSION).toInt(), true);
    }
    catch (const ApplicationException& ex) {
      qFatal("Error when running SQL scripts: %s.", qPrintable(ex.message()));
    }

    qDebugNN << LOGSEC_DB << "DB backend should be ready now.";
  }
  else {
    query_db.next();
    const int current_version = QSL(APP_DB_SCHEMA_VERSION).toInt();
    const int lowest_version = QSL(APP_DB_SCHEMA_FIRST_VERSION).toInt();
    int installed_db_schema = query_db.value(0).toString().toInt();

    if (installed_db_schema < lowest_version) {
      throw SqlException(SqlException::Type::TooOldIncompatibleDbSchema,
                         tr("this database cannot be used because it comes from too old major app version"));
    }

    if (installed_db_schema > current_version) {
      // NOTE: We have too new database version, likely from newer
      // RSS Guard. Abort.
      throw SqlException(SqlException::Type::TooNewIncompatibleDbSchema,
                         tr("database schema is too new, application requires <= %1 but %2 is installed")
                           .arg(QString::number(current_version), QString::number(installed_db_schema)));
    }

    if (installed_db_schema < current_version) {
      try {
        while (installed_db_schema != current_version) {
          const QStringList statements =
            prepareScript(APP_SQL_PATH,
                          QSL(APP_DB_UPDATE_FILE_PATTERN)
                            .arg(QString::number(installed_db_schema), QString::number(installed_db_schema + 1)),
                          database_name);

          for (const QString& statement : statements) {
            query_db.exec(statement);
          }

          // Increment the version.
          qDebugNN << LOGSEC_DB << "Updating database schema " << NONQUOTE_W_SPACE(installed_db_schema) << "->"
                   << NONQUOTE_W_SPACE_DOT(installed_db_schema + 1);

          installed_db_schema++;
        }

        setSchemaVersion(query_db, current_version, false);
        qDebugNN << LOGSEC_DB << "Database schema was updated to latest version successully.";
      }
      catch (const ApplicationException& ex) {
        qFatal("Error when updating DB schema from %d: %s.", installed_db_schema, qPrintable(ex.message()));
      }
    }
  }
}

void DatabaseDriver::setSchemaVersion(SqlQuery& query, int new_schema_version, bool empty_table) {
  if (!query.prepare(empty_table
                       ? QSL("INSERT INTO Information VALUES ('schema_version', :schema_version);")
                       : QSL("UPDATE Information SET inf_value = :schema_version WHERE inf_key = 'schema_version';"))) {
    throw ApplicationException(query.lastError().text());
  }

  query.bindValue(QSL(":schema_version"), QString::number(new_schema_version));
  query.exec();
}

QStringList DatabaseDriver::prepareScript(const QString& base_sql_folder,
                                          const QString& sql_file,
                                          const QString& database_name) {
  static QRegularExpression comment_splitter = QRegularExpression(QSL(APP_DB_COMMENT_SPLIT));
  static QRegularExpression condition(QSL("^#(if|elif)\\s+ENGINE\\s*==\\s*([A-Z][A-Z0-9_]*)$"));

  struct Conditional {
      bool m_parentActive;
      bool m_branchTaken;
      bool m_elseSeen;
      int m_line;
  };

  const QString engine = humanDriverType().toUpper();
  const QString sql_script = QString::fromUtf8(IOFactory::readFile(base_sql_folder + QDir::separator() + sql_file));
  const QStringList lines = sql_script.split(QL1C('\n'));
  QList<Conditional> conditionals;
  QString active_sql;
  bool active = true;
  const auto fail = [&](const QString& error, int line) {
    throw ApplicationException(tr("Invalid SQL preprocessor directive: %1").arg(error), sql_file, line);
  };

  // Directives occupy whole lines and are checked even inside inactive branches.
  // SQL is split only afterwards, preserving semicolons within triggers/procedures.
  for (int i = 0; i < lines.size(); ++i) {
    const QString directive = lines.at(i).trimmed();

    if (!directive.startsWith(QL1C('#'))) {
      if (active) {
        active_sql += lines.at(i) + QL1C('\n');
      }
      continue;
    }

    const auto match = condition.match(directive);

    if (match.hasMatch()) {
      const QString expected_engine = match.captured(2);
      if (expected_engine != QSL("SQLITE") && expected_engine != QSL("MARIADB")) {
        fail(tr("unknown engine '%1'").arg(expected_engine), i + 1);
      }
      const bool matches = engine == expected_engine;

      if (match.captured(1) == QSL("if")) {
        conditionals.append({active, matches, false, i + 1});
        active = active && matches;
      }
      else {
        if (conditionals.isEmpty() || conditionals.last().m_elseSeen) {
          fail(tr("unexpected #elif"), i + 1);
        }
        auto& conditional = conditionals.last();
        active = conditional.m_parentActive && !conditional.m_branchTaken && matches;
        conditional.m_branchTaken = conditional.m_branchTaken || matches;
      }
    }
    else if (directive == QSL("#else")) {
      if (conditionals.isEmpty() || conditionals.last().m_elseSeen) {
        fail(tr("unexpected #else"), i + 1);
      }
      auto& conditional = conditionals.last();
      active = conditional.m_parentActive && !conditional.m_branchTaken;
      conditional.m_branchTaken = true;
      conditional.m_elseSeen = true;
    }
    else if (directive == QSL("#endif")) {
      if (conditionals.isEmpty()) {
        fail(tr("unexpected #endif"), i + 1);
      }
      active = conditionals.takeLast().m_parentActive;
    }
    else {
      fail(directive, i + 1);
    }
  }

  if (!conditionals.isEmpty()) {
    fail(tr("missing #endif"), conditionals.last().m_line);
  }

  QStringList statements;
  for (const QString& statement : active_sql.split(comment_splitter, SPLIT_BEHAVIOR::SkipEmptyParts)) {
    if (!statement.trimmed().isEmpty()) {
      statements.append(statement.trimmed());
    }
  }

  statements =
    statements.replaceInStrings(QSL(APP_DB_NAME_PLACEHOLDER), DatabaseFactory::escapeIdentifier(database_name));
  statements = statements.replaceInStrings(QSL(APP_DB_AUTO_INC_PRIM_KEY_PLACEHOLDER), autoIncrementPrimaryKey());
  statements = statements.replaceInStrings(QSL(APP_DB_BLOB_PLACEHOLDER), blob());
  statements = statements.replaceInStrings(QSL(APP_DB_TEXT_PLACEHOLDER), text());
  statements = statements.replaceInStrings(QSL(APP_DB_FKEYS_ENABLE_PLACEHOLDER), foreignKeysEnable());
  statements = statements.replaceInStrings(QSL(APP_DB_FKEYS_DISABLE_PLACEHOLDER), foreignKeysDisable());
  statements = statements.replaceInStrings(QSL(APP_DB_COLLATE_PLACEHOLDER), collateNocase());

  return statements;
}
