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
    QString getDatabasePath();

    // NOTE: This always returns OPENED database.
    QSqlDatabase connection(const QString &connection_name = QString(),
                            bool in_memory = true);

    // Removes connection.
    void removeConnection(const QString &connection_name = QString());

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
    // returns opened connection.
    QSqlDatabase initializeInMemory();
    QSqlDatabase initializeFileBased(const QString &connection_name, bool in_memory);

    // Path to database file.
    QString m_databasePath;

    // Is database file initialized?
    bool m_fileBasedinitialized;
    bool m_inMemoryInitialized;

    // Private singleton value.
    static QPointer<DatabaseFactory> s_instance;
};

#endif // DATABASEFACTORY_H
