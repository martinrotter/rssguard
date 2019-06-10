// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/databasefactory.h"

#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

DatabaseFactory::DatabaseFactory(QObject* parent)
  : QObject(parent),
  m_activeDatabaseDriver(UsedDriver::SQLITE),
  m_mysqlDatabaseInitialized(false),
  m_sqliteFileBasedDatabaseinitialized(false),
  m_sqliteInMemoryDatabaseInitialized(false) {
  setObjectName(QSL("DatabaseFactory"));
  determineDriver();
}

qint64 DatabaseFactory::getDatabaseFileSize() const {
  if (m_activeDatabaseDriver == UsedDriver::SQLITE || m_activeDatabaseDriver == UsedDriver::SQLITE_MEMORY) {
    return QFileInfo(sqliteDatabaseFilePath()).size();
  }
  else {
    return 0;
  }
}

qint64 DatabaseFactory::getDatabaseDataSize() const {
  if (m_activeDatabaseDriver == UsedDriver::SQLITE || m_activeDatabaseDriver == UsedDriver::SQLITE_MEMORY) {
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DesiredType::FromSettings);
    qint64 result = 1;
    QSqlQuery query(database);

    if (query.exec(QSL("PRAGMA page_count;"))) {
      query.next();
      result *= query.value(0).value<qint64>();
    }
    else {
      return 0;
    }

    if (query.exec(QSL("PRAGMA page_size;"))) {
      query.next();
      result *= query.value(0).value<qint64>();
    }
    else {
      return 0;
    }

    return result;
  }
  else if (m_activeDatabaseDriver == UsedDriver::MYSQL) {
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DesiredType::FromSettings);
    qint64 result = 1;
    QSqlQuery query(database);

    if (query.exec("SELECT Round(Sum(data_length + index_length), 1) "
                   "FROM information_schema.tables "
                   "GROUP BY table_schema;")) {
      while (query.next()) {
        result *= query.value(0).value<qint64>();
      }

      return result;
    }
    else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

DatabaseFactory::MySQLError DatabaseFactory::mysqlTestConnection(const QString& hostname, int port, const QString& w_database,
                                                                 const QString& username, const QString& password) {
  QSqlDatabase database = QSqlDatabase::addDatabase(APP_DB_MYSQL_DRIVER, APP_DB_MYSQL_TEST);

  database.setHostName(hostname);
  database.setPort(port);
  database.setUserName(username);
  database.setPassword(password);
  database.setDatabaseName(w_database);

  if (database.open() && !database.lastError().isValid()) {
    QSqlQuery query(QSL("SELECT version();"), database);

    if (!query.lastError().isValid() && query.next()) {
      qDebug("Checked MySQL database, version is '%s'.", qPrintable(query.value(0).toString()));

      // Connection succeeded, clean up the mess and return OK status.
      database.close();
      return MySQLError::MySQLOk;
    }
    else {
      database.close();
      return MySQLError::MySQLUnknownError;
    }
  }
  else if (database.lastError().isValid()) {
    // Connection failed, do cleanup and return specific error code.
    return static_cast<MySQLError>(database.lastError().number());
  }
  else {
    return MySQLError::MySQLUnknownError;
  }
}

QString DatabaseFactory::mysqlInterpretErrorCode(MySQLError error_code) const {
  switch (error_code) {
    case MySQLError::MySQLOk:
      return tr("MySQL server works as expected.");

    case MySQLError::MySQLUnknownDatabase:
      return tr("Selected database does not exist (yet). It will be created. It's okay.");

    case MySQLError::MySQLCantConnect:
    case MySQLError::MySQLConnectionError:
    case MySQLError::MySQLUnknownHost:
      return tr("No MySQL server is running in the target destination.");

    case MySQLError::MySQLAccessDenied:

      //: Access to MySQL server was denied.
      return tr("Access denied. Invalid username or password used.");

    default:

      //: Unknown MySQL error arised.
      return tr("Unknown error.");
  }
}

bool DatabaseFactory::initiateRestoration(const QString& database_backup_file_path) {
  switch (m_activeDatabaseDriver) {
    case UsedDriver::SQLITE:
    case UsedDriver::SQLITE_MEMORY:
      return IOFactory::copyFile(database_backup_file_path,
                                 m_sqliteDatabaseFilePath + QDir::separator() +
                                 BACKUP_NAME_DATABASE + BACKUP_SUFFIX_DATABASE);

    default:
      return false;
  }
}

void DatabaseFactory::finishRestoration() {
  if (m_activeDatabaseDriver != UsedDriver::SQLITE && m_activeDatabaseDriver != UsedDriver::SQLITE_MEMORY) {
    return;
  }

  const QString backup_database_file = m_sqliteDatabaseFilePath + QDir::separator() + BACKUP_NAME_DATABASE + BACKUP_SUFFIX_DATABASE;

  if (QFile::exists(backup_database_file)) {
    qWarning("Backup database file '%s' was detected. Restoring it.", qPrintable(QDir::toNativeSeparators(backup_database_file)));

    if (IOFactory::copyFile(backup_database_file, m_sqliteDatabaseFilePath + QDir::separator() + APP_DB_SQLITE_FILE)) {
      QFile::remove(backup_database_file);
      qDebug("Database file was restored successully.");
    }
    else {
      qCritical("Database file was NOT restored due to error when copying the file.");
    }
  }
}

void DatabaseFactory::sqliteAssemblyDatabaseFilePath() {
  m_sqliteDatabaseFilePath = qApp->userDataFolder() + QDir::separator() + QString(APP_DB_SQLITE_PATH);
}

QSqlDatabase DatabaseFactory::sqliteInitializeInMemoryDatabase() {
  QSqlDatabase database = QSqlDatabase::addDatabase(APP_DB_SQLITE_DRIVER);

  database.setDatabaseName(QSL(":memory:"));

  if (!database.open()) {
    qFatal("In-memory SQLite database was NOT opened. Delivered error message: '%s'", qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    query_db.exec(QSL("PRAGMA encoding = \"UTF-8\""));
    query_db.exec(QSL("PRAGMA synchronous = OFF"));
    query_db.exec(QSL("PRAGMA journal_mode = MEMORY"));
    query_db.exec(QSL("PRAGMA page_size = 4096"));
    query_db.exec(QSL("PRAGMA cache_size = 16384"));
    query_db.exec(QSL("PRAGMA count_changes = OFF"));
    query_db.exec(QSL("PRAGMA temp_store = MEMORY"));

    // Sample query which checks for existence of tables.
    query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"));

    if (query_db.lastError().isValid()) {
      qWarning("Error occurred. In-memory SQLite database is not initialized. Initializing now.");
      QFile file_init(APP_SQL_PATH + QDir::separator() + APP_DB_SQLITE_INIT);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("In-memory SQLite database initialization file '%s' from directory '%s' was not found. In-memory database is uninitialized.",
               APP_DB_SQLITE_INIT,
               qPrintable(APP_SQL_PATH));
      }

      const QStringList statements = QString(file_init.readAll()).split(APP_DB_COMMENT_SPLIT, QString::SkipEmptyParts);

      database.transaction();

      foreach (const QString& statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("In-memory SQLite database initialization failed. Initialization script '%s' is not correct.", APP_DB_SQLITE_INIT);
        }
      }

      database.commit();
      qDebug("In-memory SQLite database backend should be ready now.");
    }
    else {
      query_db.next();
      qDebug("In-memory SQLite database connection seems to be established.");
      qDebug("In-memory SQLite database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    // Loading messages from file-based database.
    QSqlDatabase file_database = sqliteConnection(objectName(), DesiredType::StrictlyFileBased);
    QSqlQuery copy_contents(database);

    // Attach database.
    copy_contents.exec(QString("ATTACH DATABASE '%1' AS 'storage';").arg(file_database.databaseName()));

    // Copy all stuff.
    QStringList tables;

    if (copy_contents.exec(QSL("SELECT name FROM storage.sqlite_master WHERE type='table';"))) {
      while (copy_contents.next()) {
        tables.append(copy_contents.value(0).toString());
      }
    }
    else {
      qFatal("Cannot obtain list of table names from file-base SQLite database.");
    }

    foreach (const QString& table, tables) {
      copy_contents.exec(QString("INSERT INTO main.%1 SELECT * FROM storage.%1;").arg(table));
    }

    qDebug("Copying data from file-based database into working in-memory database.");

    // Detach database and finish.
    copy_contents.exec(QSL("DETACH 'storage'"));
    copy_contents.finish();
    query_db.finish();
  }

  // Everything is initialized now.
  m_sqliteInMemoryDatabaseInitialized = true;
  return database;
}

QSqlDatabase DatabaseFactory::sqliteInitializeFileBasedDatabase(const QString& connection_name) {
  finishRestoration();

  // Prepare file paths.
  const QDir db_path(m_sqliteDatabaseFilePath);
  QFile db_file(db_path.absoluteFilePath(APP_DB_SQLITE_FILE));

  // Check if database directory exists.
  if (!db_path.exists()) {
    if (!db_path.mkpath(db_path.absolutePath())) {
      // Failure when create database file path.
      qFatal("Directory '%s' for SQLite database file '%s' was NOT created."
             "This is HUGE problem.",
             qPrintable(db_path.absolutePath()),
             qPrintable(db_file.symLinkTarget()));
    }
  }

  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database;

  database = QSqlDatabase::addDatabase(APP_DB_SQLITE_DRIVER, connection_name);
  database.setDatabaseName(db_file.fileName());

  if (!database.open()) {
    qFatal("File-based SQLite database was NOT opened. Delivered error message: '%s'",
           qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    query_db.exec(QSL("PRAGMA encoding = \"UTF-8\""));
    query_db.exec(QSL("PRAGMA synchronous = OFF"));
    query_db.exec(QSL("PRAGMA journal_mode = MEMORY"));
    query_db.exec(QSL("PRAGMA page_size = 4096"));
    query_db.exec(QSL("PRAGMA cache_size = 16384"));
    query_db.exec(QSL("PRAGMA count_changes = OFF"));
    query_db.exec(QSL("PRAGMA temp_store = MEMORY"));

    // Sample query which checks for existence of tables.
    if (!query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"))) {
      qWarning("Error occurred. File-based SQLite database is not initialized. Initializing now.");
      QFile file_init(APP_SQL_PATH + QDir::separator() + APP_DB_SQLITE_INIT);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("SQLite database initialization file '%s' from directory '%s' was not found. File-based database is uninitialized.",
               APP_DB_SQLITE_INIT,
               qPrintable(APP_SQL_PATH));
      }

      const QStringList statements = QString(file_init.readAll()).split(APP_DB_COMMENT_SPLIT, QString::SkipEmptyParts);

      database.transaction();

      foreach (const QString& statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("File-based SQLite database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_SQLITE_INIT);
        }
      }

      database.commit();
      query_db.finish();
      qDebug("File-based SQLite database backend should be ready now.");
    }
    else {
      query_db.next();
      const QString installed_db_schema = query_db.value(0).toString();

      query_db.finish();

      if (installed_db_schema.toInt() < QString(APP_DB_SCHEMA_VERSION).toInt()) {
        if (sqliteUpdateDatabaseSchema(database, installed_db_schema)) {
          qDebug("Database schema was updated from '%s' to '%s' successully or it is already up to date.",
                 qPrintable(installed_db_schema),
                 APP_DB_SCHEMA_VERSION);
        }
        else {
          qFatal("Database schema was not updated from '%s' to '%s' successully.",
                 qPrintable(installed_db_schema),
                 APP_DB_SCHEMA_VERSION);
        }
      }

      qDebug("File-based SQLite database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
      qDebug("File-based SQLite database has version '%s'.", qPrintable(installed_db_schema));
    }
  }

  // Everything is initialized now.
  m_sqliteFileBasedDatabaseinitialized = true;
  return database;
}

QString DatabaseFactory::sqliteDatabaseFilePath() const {
  return m_sqliteDatabaseFilePath + QDir::separator() + APP_DB_SQLITE_FILE;
}

bool DatabaseFactory::sqliteUpdateDatabaseSchema(const QSqlDatabase& database, const QString& source_db_schema_version) {
  int working_version = QString(source_db_schema_version).remove('.').toInt();
  const int current_version = QString(APP_DB_SCHEMA_VERSION).remove('.').toInt();

  // Now, it would be good to create backup of SQLite DB file.
  if (IOFactory::copyFile(sqliteDatabaseFilePath(), sqliteDatabaseFilePath() + ".bak")) {
    qDebug("Creating backup of SQLite DB file.");
  }
  else {
    qFatal("Creation of backup SQLite DB file failed.");
  }

  while (working_version != current_version) {
    const QString update_file_name = QString(APP_SQL_PATH) + QDir::separator() +
                                     QString(APP_DB_UPDATE_FILE_PATTERN).arg(QSL("sqlite"),
                                                                             QString::number(working_version),
                                                                             QString::number(working_version + 1));

    if (!QFile::exists(update_file_name)) {
      qFatal("Updating of database schema failed. File '%s' does not exist.", qPrintable(QDir::toNativeSeparators(update_file_name)));
    }

    QFile update_file_handle(update_file_name);

    if (!update_file_handle.open(QIODevice::Text | QIODevice::ReadOnly | QIODevice::Unbuffered)) {
      qFatal("Updating of database schema failed. File '%s' cannot be opened.", qPrintable(QDir::toNativeSeparators(update_file_name)));
    }

    const QStringList statements = QString(update_file_handle.readAll()).split(APP_DB_COMMENT_SPLIT, QString::SkipEmptyParts);

    foreach (const QString& statement, statements) {
      QSqlQuery query = database.exec(statement);

      if (query.lastError().isValid()) {
        qFatal("Query for updating database schema failed: '%s'.", qPrintable(query.lastError().text()));
      }
    }

    // Increment the version.
    qDebug("Updating database schema: '%d' -> '%d'.", working_version, working_version + 1);
    working_version++;
  }

  return true;
}

bool DatabaseFactory::mysqlUpdateDatabaseSchema(const QSqlDatabase& database,
                                                const QString& source_db_schema_version,
                                                const QString& db_name) {
  int working_version = QString(source_db_schema_version).remove('.').toInt();
  const int current_version = QString(APP_DB_SCHEMA_VERSION).remove('.').toInt();

  while (working_version != current_version) {
    const QString update_file_name = QString(APP_SQL_PATH) + QDir::separator() +
                                     QString(APP_DB_UPDATE_FILE_PATTERN).arg(QSL("mysql"),
                                                                             QString::number(working_version),
                                                                             QString::number(working_version + 1));

    if (!QFile::exists(update_file_name)) {
      qFatal("Updating of database schema failed. File '%s' does not exist.", qPrintable(QDir::toNativeSeparators(update_file_name)));
    }

    QFile update_file_handle(update_file_name);

    if (!update_file_handle.open(QIODevice::Text | QIODevice::ReadOnly | QIODevice::Unbuffered)) {
      qFatal("Updating of database schema failed. File '%s' cannot be opened.", qPrintable(QDir::toNativeSeparators(update_file_name)));
    }

    QStringList statements = QString(update_file_handle.readAll()).split(APP_DB_COMMENT_SPLIT, QString::SkipEmptyParts);

    foreach (QString statement, statements) {
      QSqlQuery query = database.exec(statement.replace(APP_DB_NAME_PLACEHOLDER, db_name));

      if (query.lastError().isValid()) {
        qFatal("Query for updating database schema failed: '%s'.", qPrintable(query.lastError().text()));
      }
    }

    // Increment the version.
    qDebug("Updating database schema: '%d' -> '%d'.", working_version, working_version + 1);
    working_version++;
  }

  return true;
}

QSqlDatabase DatabaseFactory::connection(const QString& connection_name, DesiredType desired_type) {
  switch (m_activeDatabaseDriver) {
    case UsedDriver::MYSQL:
      return mysqlConnection(connection_name);

    case UsedDriver::SQLITE:
    case UsedDriver::SQLITE_MEMORY:
    default:
      return sqliteConnection(connection_name, desired_type);
  }
}

QString DatabaseFactory::humanDriverName(DatabaseFactory::UsedDriver driver) const {
  switch (driver) {
    case UsedDriver::MYSQL:
      return tr("MySQL/MariaDB (dedicated database)");

    case UsedDriver::SQLITE:
    case UsedDriver::SQLITE_MEMORY:
    default:
      return tr("SQLite (embedded database)");
  }
}

QString DatabaseFactory::humanDriverName(const QString& driver_code) const {
  if (driver_code == APP_DB_SQLITE_DRIVER) {
    return humanDriverName(UsedDriver::SQLITE);
  }
  else if (driver_code == APP_DB_MYSQL_DRIVER) {
    return humanDriverName(UsedDriver::MYSQL);
  }
  else {
    return humanDriverName(UsedDriver::SQLITE);
  }
}

void DatabaseFactory::removeConnection(const QString& connection_name) {
  qDebug("Removing database connection '%s'.", qPrintable(connection_name));
  QSqlDatabase::removeDatabase(connection_name);
}

QString DatabaseFactory::obtainBeginTransactionSql() const {
  if (m_activeDatabaseDriver == UsedDriver::SQLITE || m_activeDatabaseDriver == UsedDriver::SQLITE_MEMORY) {
    return QSL("BEGIN IMMEDIATE TRANSACTION;");
  }
  else {
    return QSL("START TRANSACTION;");
  }
}

void DatabaseFactory::sqliteSaveMemoryDatabase() {
  qDebug("Saving in-memory working database back to persistent file-based storage.");
  QSqlDatabase database = sqliteConnection(objectName(), DesiredType::StrictlyInMemory);
  QSqlDatabase file_database = sqliteConnection(objectName(), DesiredType::StrictlyFileBased);
  QSqlQuery copy_contents(database);

  // Attach database.
  copy_contents.exec(QString(QSL("ATTACH DATABASE '%1' AS 'storage';")).arg(file_database.databaseName()));

  // Copy all stuff.
  QStringList tables;

  if (copy_contents.exec(QSL("SELECT name FROM storage.sqlite_master WHERE type='table';"))) {
    while (copy_contents.next()) {
      tables.append(copy_contents.value(0).toString());
    }
  }
  else {
    qFatal("Cannot obtain list of table names from file-base SQLite database.");
  }

  foreach (const QString& table, tables) {
    copy_contents.exec(QString(QSL("DELETE FROM storage.%1;")).arg(table));
    copy_contents.exec(QString(QSL("INSERT INTO storage.%1 SELECT * FROM main.%1;")).arg(table));
  }

  // Detach database and finish.
  copy_contents.exec(QSL("DETACH 'storage'"));
  copy_contents.finish();
}

void DatabaseFactory::determineDriver() {
  const QString db_driver = qApp->settings()->value(GROUP(Database), SETTING(Database::ActiveDriver)).toString();

  if (db_driver == APP_DB_MYSQL_DRIVER && QSqlDatabase::isDriverAvailable(APP_DB_SQLITE_DRIVER)) {
    // User wants to use MySQL and MySQL is actually available. Use it.
    m_activeDatabaseDriver = UsedDriver::MYSQL;
    qDebug("Working database source was as MySQL database.");
  }
  else {
    // User wants to use SQLite, which is always available. Check if file-based
    // or in-memory database will be used.
    if (qApp->settings()->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool()) {
      // Use in-memory SQLite database.
      m_activeDatabaseDriver = UsedDriver::SQLITE_MEMORY;
      qDebug("Working database source was determined as SQLite in-memory database.");
    }
    else {
      // Use strictly file-base SQLite database.
      m_activeDatabaseDriver = UsedDriver::SQLITE;
      qDebug("Working database source was determined as SQLite file-based database.");
    }

    sqliteAssemblyDatabaseFilePath();
  }
}

DatabaseFactory::UsedDriver DatabaseFactory::activeDatabaseDriver() const {
  return m_activeDatabaseDriver;
}

QSqlDatabase DatabaseFactory::mysqlConnection(const QString& connection_name) {
  if (!m_mysqlDatabaseInitialized) {
    // Return initialized database.
    return mysqlInitializeDatabase(connection_name);
  }
  else {
    QSqlDatabase database;

    if (QSqlDatabase::contains(connection_name)) {
      qDebug("MySQL connection '%s' is already active.", qPrintable(connection_name));

      // This database connection was added previously, no need to
      // setup its properties.
      database = QSqlDatabase::database(connection_name);
    }
    else {
      // Database connection with this name does not exist
      // yet, add it and set it up.
      database = QSqlDatabase::addDatabase(APP_DB_MYSQL_DRIVER, connection_name);
      database.setHostName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
      database.setPort(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());
      database.setUserName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
      database.setPassword(qApp->settings()->password(GROUP(Database), SETTING(Database::MySQLPassword)).toString());
      database.setDatabaseName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString());
    }

    if (!database.isOpen() && !database.open()) {
      qFatal("MySQL database was NOT opened. Delivered error message: '%s'.",
             qPrintable(database.lastError().text()));
    }
    else {
      qDebug("MySQL database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
    }

    return database;
  }
}

QSqlDatabase DatabaseFactory::mysqlInitializeDatabase(const QString& connection_name) {
  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database = QSqlDatabase::addDatabase(APP_DB_MYSQL_DRIVER, connection_name);
  const QString database_name = qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString();

  database.setHostName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
  database.setPort(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());
  database.setUserName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
  database.setPassword(qApp->settings()->password(GROUP(Database), SETTING(Database::MySQLPassword)).toString());

  if (!database.open()) {
    qCritical("MySQL database was NOT opened. Delivered error message: '%s'", qPrintable(database.lastError().text()));

    // Now, we will display error warning and return SQLite connection.
    // Also, we set the SQLite driver as active one.
    qApp->settings()->setValue(GROUP(Database), Database::ActiveDriver, APP_DB_SQLITE_DRIVER);
    determineDriver();
    MessageBox::show(nullptr, QMessageBox::Critical, tr("MySQL database not available"),
                     tr("%1 cannot use MySQL storage, it is not available. %1 is now switching to SQLite database. Start your MySQL server "
                        "and make adjustments in application settings.").arg(APP_NAME));
    return connection(objectName(), DesiredType::FromSettings);
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);

    if (!query_db.exec(QString("USE %1").arg(database_name))
        || !query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"))) {
      // If no "rssguard" database exists or schema version is wrong, then initialize it.
      qWarning("Error occurred. MySQL database is not initialized. Initializing now.");
      QFile file_init(APP_SQL_PATH + QDir::separator() + APP_DB_MYSQL_INIT);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("MySQL database initialization file '%s' from directory '%s' was not found. File-based database is uninitialized.",
               APP_DB_MYSQL_INIT,
               qPrintable(APP_SQL_PATH));
      }

      const QStringList statements = QString(file_init.readAll()).split(APP_DB_COMMENT_SPLIT, QString::SkipEmptyParts);

      database.transaction();

      foreach (QString statement, statements) {
        // Assign real database name and run the query.
        query_db.exec(statement.replace(APP_DB_NAME_PLACEHOLDER, database_name));

        if (query_db.lastError().isValid()) {
          qFatal("MySQL database initialization failed. Initialization script '%s' is not correct. Error : '%s'.",
                 APP_DB_MYSQL_INIT, qPrintable(query_db.lastError().databaseText()));
        }
      }

      database.commit();
      qDebug("MySQL database backend should be ready now.");
    }
    else {
      // Database was previously initialized. Now just check the schema version.
      query_db.next();
      const QString installed_db_schema = query_db.value(0).toString();

      if (installed_db_schema < APP_DB_SCHEMA_VERSION) {
        if (mysqlUpdateDatabaseSchema(database, installed_db_schema, database_name)) {
          qDebug("Database schema was updated from '%s' to '%s' successully or it is already up to date.",
                 qPrintable(installed_db_schema),
                 APP_DB_SCHEMA_VERSION);
        }
        else {
          qFatal("Database schema was not updated from '%s' to '%s' successully.",
                 qPrintable(installed_db_schema),
                 APP_DB_SCHEMA_VERSION);
        }
      }
    }

    query_db.finish();
  }

  // Everything is initialized now.
  m_mysqlDatabaseInitialized = true;
  return database;
}

bool DatabaseFactory::mysqlVacuumDatabase() {
  QSqlDatabase database = mysqlConnection(objectName());
  QSqlQuery query_vacuum(database);

  return query_vacuum.exec(QSL("OPTIMIZE TABLE Feeds;")) && query_vacuum.exec(QSL("OPTIMIZE TABLE Messages;"));
}

QSqlDatabase DatabaseFactory::sqliteConnection(const QString& connection_name, DatabaseFactory::DesiredType desired_type) {
  if (desired_type == DesiredType::StrictlyInMemory ||
      (desired_type == DesiredType::FromSettings && m_activeDatabaseDriver == UsedDriver::SQLITE_MEMORY)) {
    // We request in-memory database (either user explicitly
    // needs in-memory database or it was enabled in the settings).
    if (!m_sqliteInMemoryDatabaseInitialized) {
      // It is not initialized yet.
      return sqliteInitializeInMemoryDatabase();
    }
    else {
      QSqlDatabase database = QSqlDatabase::database();

      database.setDatabaseName(QSL(":memory:"));

      if (!database.isOpen() && !database.open()) {
        qFatal("In-memory SQLite database was NOT opened. Delivered error message: '%s'.",
               qPrintable(database.lastError().text()));
      }
      else {
        qDebug("In-memory SQLite database connection '%s' seems to be established.", qPrintable(connection_name));
      }

      return database;
    }
  }
  else {
    // We request file-based database.
    if (!m_sqliteFileBasedDatabaseinitialized) {
      // File-based database is not yet initialised.
      return sqliteInitializeFileBasedDatabase(connection_name);
    }
    else {
      QSqlDatabase database;

      if (QSqlDatabase::contains(connection_name)) {
        qDebug("SQLite connection '%s' is already active.", qPrintable(connection_name));

        // This database connection was added previously, no need to
        // setup its properties.
        database = QSqlDatabase::database(connection_name);
      }
      else {
        // Database connection with this name does not exist
        // yet, add it and set it up.
        database = QSqlDatabase::addDatabase(APP_DB_SQLITE_DRIVER, connection_name);
        const QDir db_path(m_sqliteDatabaseFilePath);
        QFile db_file(db_path.absoluteFilePath(APP_DB_SQLITE_FILE));

        // Setup database file path.
        database.setDatabaseName(db_file.fileName());
      }

      if (!database.isOpen() && !database.open()) {
        qFatal("File-based SQLite database was NOT opened. Delivered error message: '%s'.",
               qPrintable(database.lastError().text()));
      }
      else {
        qDebug("File-based SQLite database connection '%s' to file '%s' seems to be established.",
               qPrintable(connection_name),
               qPrintable(QDir::toNativeSeparators(database.databaseName())));
      }

      return database;
    }
  }
}

bool DatabaseFactory::sqliteVacuumDatabase() {
  QSqlDatabase database;

  if (m_activeDatabaseDriver == UsedDriver::SQLITE) {
    database = sqliteConnection(objectName(), DesiredType::StrictlyFileBased);
  }
  else if (m_activeDatabaseDriver == UsedDriver::SQLITE_MEMORY) {
    sqliteSaveMemoryDatabase();
    database = sqliteConnection(objectName(), DesiredType::StrictlyFileBased);
  }
  else {
    return false;
  }

  QSqlQuery query_vacuum(database);

  return query_vacuum.exec(QSL("VACUUM"));
}

void DatabaseFactory::saveDatabase() {
  switch (m_activeDatabaseDriver) {
    case UsedDriver::SQLITE_MEMORY:
      sqliteSaveMemoryDatabase();
      break;

    default:
      break;
  }
}

bool DatabaseFactory::vacuumDatabase() {
  switch (m_activeDatabaseDriver) {
    case UsedDriver::SQLITE_MEMORY:
    case UsedDriver::SQLITE:
      return sqliteVacuumDatabase();

    case UsedDriver::MYSQL:
      return mysqlVacuumDatabase();

    default:
      return false;
  }
}
