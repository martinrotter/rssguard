// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasedriver.h"

#include "database/sqlquery.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/messagebox.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/thread.h"

#include <QDir>
#include <QRegularExpression>
#include <QSqlError>
#include <QThread>

DatabaseDriver::DatabaseDriver(QObject* parent) : QObject(parent), m_databaseInitialized(false) {}

QSqlDatabase DatabaseDriver::threadSafeConnection(const QString& connection_name) {
  qlonglong thread_id = getThreadID();
  bool is_main_thread = QThread::currentThread() == qApp->thread();
  QSqlDatabase database =
    connection(is_main_thread ? connection_name : QSL("%1_%2").arg(connection_name, QString::number(thread_id)));

  return database;
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
      qFatal("Database was NOT opened. Delivered error message: '%s'", qPrintable(database.lastError().text()));
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

    if (!database.isOpen() && !database.open()) {
      qFatal("Database was NOT opened. Delivered error message: '%s'.", qPrintable(database.lastError().text()));
    }
    else {
      qDebugNN << LOGSEC_DB << "Database connection" << QUOTE_W_SPACE(connection_name) << "to DB"
               << QUOTE_W_SPACE(database.databaseName()) << "seems to be established.";
    }

    SqlQuery query_db(database);
    setPragmas(query_db);
  }

  return database;
}

void DatabaseDriver::beforeAddDatabase() {}

void DatabaseDriver::afterAddDatabase(QSqlDatabase& database, bool was_initialized) {
  Q_UNUSED(database)
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
      const QStringList statements =
        prepareScript(APP_SQL_PATH, QSL(APP_DB_INIT_FILE_PATTERN).arg(ddlFilePrefix()), database_name);

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
      // This database comes from older major RSS Guard version.
      MsgBox::show(nullptr,
                   QMessageBox::Icon::Critical,
                   tr("Cannot use this DB file"),
                   tr("The database file you provided cannot be used because it comes from old major version of %1.")
                     .arg(QSL(APP_NAME)));
      throw ApplicationException(tr("this database file cannot be used because it comes from old major app version"));
    }

    if (installed_db_schema < current_version) {
      try {
        while (installed_db_schema != current_version) {
          const QStringList statements = prepareScript(APP_SQL_PATH,
                                                       QSL(APP_DB_UPDATE_FILE_PATTERN)
                                                         .arg(ddlFilePrefix(),
                                                              QString::number(installed_db_schema),
                                                              QString::number(installed_db_schema + 1)),
                                                       database_name);

          for (const QString& statement : statements) {
            query_db.exec(statement);
          }

          // Increment the version.
          qDebugNN << LOGSEC_DB << "Updating database schema " << QUOTE_W_SPACE(installed_db_schema) << "->"
                   << QUOTE_W_SPACE_DOT(installed_db_schema + 1);

          installed_db_schema++;
        }

        setSchemaVersion(query_db, current_version, false);

        qDebugNN << LOGSEC_DB << "Database schema was updated from" << QUOTE_W_SPACE(installed_db_schema) << "to"
                 << QUOTE_W_SPACE(APP_DB_SCHEMA_VERSION) << "successully.";
      }
      catch (const ApplicationException& ex) {
        qFatal("Error when updating DB schema from %d: %s.", installed_db_schema, qPrintable(ex.message()));
      }
    }
    else if (installed_db_schema > current_version) {
      // NOTE: We have too new database version, likely from newer
      // RSS Guard. Abort.
      qFatal("Database schema is too new. Application requires <= %d but %d is installed.",
             current_version,
             installed_db_schema);
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

  QStringList statements;
  QString next_file = base_sql_folder + QDir::separator() + sql_file;
  QString sql_script = QString::fromUtf8(IOFactory::readFile(next_file));
  QStringList new_statements = sql_script.split(comment_splitter, SPLIT_BEHAVIOR::SkipEmptyParts);

  for (int i = 0; i < new_statements.size(); i++) {
    if (new_statements.at(i).startsWith(QSL(APP_DB_INCLUDE_PLACEHOLDER))) {
      // We include another file.
      QString included_file_name = new_statements.at(i).mid(QSL(APP_DB_INCLUDE_PLACEHOLDER).size() + 1).simplified();

      QString included_file = base_sql_folder + QDir::separator() + included_file_name;
      QString included_sql_script = QString::fromUtf8(IOFactory::readFile(included_file));
      QStringList included_statements = included_sql_script.split(comment_splitter, SPLIT_BEHAVIOR::SkipEmptyParts);

      statements << included_statements;
    }
    else {
      statements << new_statements.at(i);
    }
  }

  statements = statements.replaceInStrings(QSL(APP_DB_NAME_PLACEHOLDER), database_name);
  statements = statements.replaceInStrings(QSL(APP_DB_AUTO_INC_PRIM_KEY_PLACEHOLDER), autoIncrementPrimaryKey());
  statements = statements.replaceInStrings(QSL(APP_DB_BLOB_PLACEHOLDER), blob());
  statements = statements.replaceInStrings(QSL(APP_DB_TEXT_PLACEHOLDER), text());
  statements = statements.replaceInStrings(QSL(APP_DB_FKEYS_ENABLE_PLACEHOLDER), foreignKeysEnable());
  statements = statements.replaceInStrings(QSL(APP_DB_FKEYS_DISABLE_PLACEHOLDER), foreignKeysDisable());
  statements = statements.replaceInStrings(QSL(APP_DB_COLLATE_PLACEHOLDER), collateNocase());

  return statements;
}
