#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>
#include <QPointer>
#include <QSqlDatabase>

// TODO: přidat podporu pro mysql
// nemužu mit stejny SQL kod pro mysql a sqlite
// ale musim docilit aby oba kody SQL delaly tabulky
// se stejnymi atributy - stejnymi nazvy sloupcu
// PROBLEMY se mysql: nutno pridat AUTO_INCREMENT u int primary keyů
// taky bacha na nazvy sloupců, třeba
// key a read sou klicovy slouva a fejlne to tak
// taky bacha ze typ TEXT nemuze bejt dost dobre
// pouzitej v UNIQUE CHECK, misto toho se da pouzit treba
// VARCHAR (100)


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
    int mysqlTestConnection(const QString &hostname, int port,
                            const QString &usernam, const QString &password);


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
