// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasedriver.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/thread.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

DatabaseDriver::DatabaseDriver(QObject* parent) : QObject(parent) {}

QSqlDatabase DatabaseDriver::threadSafeConnection(const QString& connection_name, DesiredStorageType desired_type) {
  qlonglong thread_id = getThreadID();
  bool is_main_thread = QThread::currentThread() == qApp->thread();

  QSqlDatabase database =
    connection(is_main_thread ? connection_name : QSL("db_connection_%1").arg(thread_id), desired_type);

  return database;
}

void DatabaseDriver::updateDatabaseSchema(QSqlQuery& query,
                                          int source_db_schema_version,
                                          const QString& database_name) {
  const int current_version = QSL(APP_DB_SCHEMA_VERSION).toInt();

  while (source_db_schema_version != current_version) {
    const QStringList statements = prepareScript(APP_SQL_PATH,
                                                 QSL(APP_DB_UPDATE_FILE_PATTERN)
                                                   .arg(ddlFilePrefix(),
                                                        QString::number(source_db_schema_version),
                                                        QString::number(source_db_schema_version + 1)),
                                                 database_name);

    for (const QString& statement : statements) {
      if (!query.exec(statement) && query.lastError().isValid()) {
        throw ApplicationException(query.lastError().text());
      }
    }

    // Increment the version.
    qDebugNN << LOGSEC_DB << "Updating database schema " << QUOTE_W_SPACE(source_db_schema_version) << "->"
             << QUOTE_W_SPACE_DOT(source_db_schema_version + 1);

    source_db_schema_version++;
  }

  setSchemaVersion(query, current_version, false);
}

void DatabaseDriver::setSchemaVersion(QSqlQuery& query, int new_schema_version, bool empty_table) {
  if (!query.prepare(empty_table
                       ? QSL("INSERT INTO Information VALUES ('schema_version', :schema_version);")
                       : QSL("UPDATE Information SET inf_value = :schema_version WHERE inf_key = 'schema_version';"))) {
    throw ApplicationException(query.lastError().text());
  }

  query.bindValue(QSL(":schema_version"), QString::number(new_schema_version));
  if (!query.exec()) {
    throw ApplicationException(query.lastError().text());
  }
}

QStringList DatabaseDriver::prepareScript(const QString& base_sql_folder,
                                          const QString& sql_file,
                                          const QString& database_name) {
  QStringList statements;
  QString next_file = base_sql_folder + QDir::separator() + sql_file;
  QString sql_script = QString::fromUtf8(IOFactory::readFile(next_file));
  QStringList new_statements = sql_script.split(QSL(APP_DB_COMMENT_SPLIT),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                QString::SplitBehavior::SkipEmptyParts);
#endif

  for (int i = 0; i < new_statements.size(); i++) {
    if (new_statements.at(i).startsWith(QSL(APP_DB_INCLUDE_PLACEHOLDER))) {
      // We include another file.
      QString included_file_name = new_statements.at(i).mid(QSL(APP_DB_INCLUDE_PLACEHOLDER).size() + 1).simplified();

      QString included_file = base_sql_folder + QDir::separator() + included_file_name;
      QString included_sql_script = QString::fromUtf8(IOFactory::readFile(included_file));
      QStringList included_statements = included_sql_script.split(QSL(APP_DB_COMMENT_SPLIT),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                                  Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                                  QString::SplitBehavior::SkipEmptyParts);
#endif

      statements << included_statements;
    }
    else {
      statements << new_statements.at(i);
    }
  }

  statements = statements.replaceInStrings(QSL(APP_DB_NAME_PLACEHOLDER), database_name);
  statements = statements.replaceInStrings(QSL(APP_DB_AUTO_INC_PRIM_KEY_PLACEHOLDER), autoIncrementPrimaryKey());
  statements = statements.replaceInStrings(QSL(APP_DB_BLOB_PLACEHOLDER), blob());

  return statements;
}
