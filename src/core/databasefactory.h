#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>
#include <QPointer>
#include <QSqlDatabase>


class DatabaseFactory : public QObject {
    Q_OBJECT

  public:
    // Destructor.
    virtual ~DatabaseFactory();

    // Returns absolute file path to database file.
    inline QString getDatabasePath() {
      return m_databaseFilePath;
    }

    // If in-memory is true, then :memory: database is returned
    // In-memory database is DEFAULT database.
    // NOTE: This always returns OPENED database.
    QSqlDatabase connection(const QString &connection_name = QString(),
                            bool in_memory = true);

    // Removes connection.
    inline void removeConnection(const QString &connection_name = QString()) {
      qDebug("Removing database connection '%s'.", qPrintable(connection_name));

      QSqlDatabase::removeDatabase(connection_name);
    }

    // Performs saving of items from in-memory database
    // to file-based database.
    void saveMemoryDatabase();

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

    // Private singleton value.
    static QPointer<DatabaseFactory> s_instance;
};

#endif // DATABASEFACTORY_H
