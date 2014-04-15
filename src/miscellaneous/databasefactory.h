// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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
#include <QPointer>
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
      MySQLOk                 = 0,
      MySQLAccessDenied       = 1045,
      MySQLConnectionError    = 2002,
      MySQLCantConnect        = 2003,
      MySQLUnknownHost        = 2005
    };

    //
    // GENERAL stuff.
    //

    // Destructor.
    virtual ~DatabaseFactory();

    // If in-memory is true, then :memory: database is returned
    // In-memory database is DEFAULT database.
    // NOTE: This always returns OPENED database.
    QSqlDatabase connection(const QString &connection_name,
                            DesiredType desired_type);

    // Removes connection.
    void removeConnection(const QString &connection_name = QString());

    // Performs any needed database-related operation to be done
    // to gracefully exit the application.
    void saveDatabase();

    // Performs cleanup of the database.
    bool vacuumDatabase();

    // Singleton getter.
    static DatabaseFactory *instance();

    //
    // MySQL stuff.
    //

    // Tests if database connection with given data
    // can be established and returns 0 if it can.
    // Otherwise returns MySQL-specific error code.
    MySQLError mysqlTestConnection(const QString &hostname, int port,
                                   const QString &username, const QString &password);

    QString mysqlInterpretErrorCode(MySQLError error_code);

  private:
    //
    // GENERAL stuff.
    //

    // Constructor.
    explicit DatabaseFactory(QObject *parent = 0);

    // Decides which database backend will be used in this
    // application session.
    void determineDriver();

    // Private singleton value.
    static QPointer<DatabaseFactory> s_instance;

    // Holds the type of currently activated database backend.
    UsedDriver m_activeDatabaseDriver;

    //
    // MYSQL stuff.
    //

    // Returns (always OPENED) MySQL database connection.
    QSqlDatabase mysqlConnection(const QString &connection_name);

    // Initializes MySQL database.
    QSqlDatabase mysqlInitializeDatabase(const QString &connection_name);

    // True if MySQL database is fully initialized for use,
    // otherwise false.
    bool m_mysqlDatabaseInitialized;

    //
    // SQLITE stuff.
    //

    QSqlDatabase sqliteConnection(const QString &connection_name,
                                  DesiredType desired_type);

    // Runs "VACUUM" on the database.
    bool sqliteVacuumDatabase();

    // Performs saving of items from in-memory database
    // to file-based database.
    void sqliteSaveMemoryDatabase();

    // Assemblies database file path.
    void sqliteAssemblyDatabaseFilePath();

    // Creates new connection, initializes database and
    // returns opened connections.
    QSqlDatabase sqliteInitializeInMemoryDatabase();
    QSqlDatabase sqliteInitializeFileBasedDatabase(const QString &connection_name);

    // Path to database file.
    QString m_sqliteDatabaseFilePath;

    // Is database file initialized?
    bool m_sqliteFileBasedDatabaseinitialized;
    bool m_sqliteInMemoryDatabaseInitialized;
};

#endif // DATABASEFACTORY_H
