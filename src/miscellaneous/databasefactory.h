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

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>
#include <QSqlDatabase>


class DatabaseFactory : public QObject {
		Q_OBJECT

	public:
		// Describes available typos of database backend.
		enum UsedDriver {
			SQLITE,
			SQLITE_MEMORY,
			MYSQL
		};

		// Describes what type of database user wants.
		enum DesiredType {
			StrictlyFileBased,
			StrictlyInMemory,
			FromSettings
		};

		// Describes possible MySQL-specific errors.
		enum MySQLError {
			MySQLOk               = 0,
			MySQLUnknownError     = 1,
			MySQLAccessDenied     = 1045,
			MySQLUnknownDatabase  = 1049,
			MySQLConnectionError  = 2002,
			MySQLCantConnect      = 2003,
			MySQLUnknownHost      = 2005
		};

		//
		// GENERAL stuff.
		//

		// Constructor.
		explicit DatabaseFactory(QObject* parent = 0);

		// Destructor.
		virtual ~DatabaseFactory();

		// Returns size of DB file.
		qint64 getDatabaseFileSize() const;

		// Returns size of data contained in the DB file.
		qint64 getDatabaseDataSize() const;

		// If in-memory is true, then :memory: database is returned
		// In-memory database is DEFAULT database.
		// NOTE: This always returns OPENED database.
		QSqlDatabase connection(const QString& connection_name, DesiredType desired_type = FromSettings);

		QString humanDriverName(UsedDriver driver) const;
		QString humanDriverName(const QString& driver_code) const;

		// Removes connection.
		void removeConnection(const QString& connection_name = QString());

		QString obtainBeginTransactionSql() const;

		// Performs any needed database-related operation to be done
		// to gracefully exit the application.
		void saveDatabase();

		// Performs cleanup of the database.
		bool vacuumDatabase();

		// Returns identification of currently active database driver.
		UsedDriver activeDatabaseDriver() const;

		// Copies selected backup database (file) to active database path.
		bool initiateRestoration(const QString& database_backup_file_path);

		// Finishes restoration from backup file.
		void finishRestoration();

		//
		// SQLITE stuff.
		//
		QString sqliteDatabaseFilePath() const;

		//
		// MySQL stuff.
		//

		// Tests if database connection with given data
		// can be established and returns 0 if it can.
		// Otherwise returns MySQL-specific error code.
		MySQLError mysqlTestConnection(const QString& hostname, int port, const QString& w_database,
		                               const QString& username, const QString& password);

		// Interprets MySQL error code.
		QString mysqlInterpretErrorCode(MySQLError error_code) const;

	private:
		//
		// GENERAL stuff.
		//

		// Decides which database backend will be used in this
		// application session.
		void determineDriver();

		// Holds the type of currently activated database backend.
		UsedDriver m_activeDatabaseDriver;

		//
		// MYSQL stuff.
		//

		// Returns (always OPENED) MySQL database connection.
		QSqlDatabase mysqlConnection(const QString& connection_name);

		// Initializes MySQL database.
		QSqlDatabase mysqlInitializeDatabase(const QString& connection_name);

		// Updates database schema.
		bool mysqlUpdateDatabaseSchema(QSqlDatabase database, const QString& source_db_schema_version, const QString& db_name);

		// Runs "VACUUM" on the database.
		bool mysqlVacuumDatabase();

		// True if MySQL database is fully initialized for use,
		// otherwise false.
		bool m_mysqlDatabaseInitialized;

		//
		// SQLITE stuff.
		//

		QSqlDatabase sqliteConnection(const QString& connection_name, DesiredType desired_type);

		// Runs "VACUUM" on the database.
		bool sqliteVacuumDatabase();

		// Performs saving of items from in-memory database
		// to file-based database.
		void sqliteSaveMemoryDatabase();

		// Assemblies database file path.
		void sqliteAssemblyDatabaseFilePath();

		// Updates database schema.
		bool sqliteUpdateDatabaseSchema(QSqlDatabase database, const QString& source_db_schema_version);

		// Creates new connection, initializes database and
		// returns opened connections.
		QSqlDatabase sqliteInitializeInMemoryDatabase();
		QSqlDatabase sqliteInitializeFileBasedDatabase(const QString& connection_name);

		// Path to database file.
		QString m_sqliteDatabaseFilePath;

		// Is database file initialized?
		bool m_sqliteFileBasedDatabaseinitialized;
		bool m_sqliteInMemoryDatabaseInitialized;
};

#endif // DATABASEFACTORY_H
