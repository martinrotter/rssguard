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
    // Describes what type of database user wants.
    enum DesiredType {
      StrictlyFileBased,
      StrictlyInMemory,
      FromSettings
    };

    // Destructor.
    virtual ~DatabaseFactory();

    // Returns absolute file path to database file.
    inline QString databaseFilePath() {
      return m_databaseFilePath;
    }

    // If in-memory is true, then :memory: database is returned
    // In-memory database is DEFAULT database.
    // NOTE: This always returns OPENED database.
    QSqlDatabase connection(const QString &connection_name,
                            DesiredType desired_type);

    // Removes connection.
    void removeConnection(const QString &connection_name = QString());

    // Performs saving of items from in-memory database
    // to file-based database.
    void saveMemoryDatabase();

    // Sets m_inMemoryEnabled according to user settings.
    void determineInMemoryDatabase();

    // Performs "VACUUM" on the database and
    // returns true of operation succeeded.
    bool vacuumDatabase();

    // Returns whether in-memory database feature is currently
    // used.
    inline bool usingInMemoryDatabase() const {
      return m_inMemoryEnabled;
    }

    // Singleton getter.
    static DatabaseFactory *instance();

  private:
    // Conctructor.
    explicit DatabaseFactory(QObject *parent = 0);

    // Assemblies database file path.
    void assemblyDatabaseFilePath();

    // Creates new connection, initializes database and
    // returns opened connections.
    QSqlDatabase initializeInMemoryDatabase();
    QSqlDatabase initializeFileBasedDatabase(const QString &connection_name);

    // Path to database file.
    QString m_databaseFilePath;

    // Is database file initialized?
    bool m_fileBasedinitialized;
    bool m_inMemoryInitialized;

    // Is true when user selected in-memory database.
    // NOTE: This can be changed only on application startup.
    bool m_inMemoryEnabled;

    // Private singleton value.
    static QPointer<DatabaseFactory> s_instance;
};

#endif // DATABASEFACTORY_H
