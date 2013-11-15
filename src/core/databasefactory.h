#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>
#include <QPointer>
#include <QSqlDatabase>


class DatabaseFactory : public QObject {
    Q_OBJECT

  private:
    // Conctructor.
    explicit DatabaseFactory(QObject *parent = 0);

    // Assemblies database file path.
    void assemblyDatabaseFilePath();

    // Returns true if database was initialized, otherwise false.
    QSqlDatabase initialize(const QString &connection_name);

    // Path to database file.
    QString m_databasePath;

    // Private singleton value.
    static QPointer<DatabaseFactory> s_instance;

  public:
    // Destructor.
    virtual ~DatabaseFactory();

    // Returns absolute file path to database file.
    QString getDatabasePath();

    // Database manipulators.
    QSqlDatabase addConnection(const QString &connection_name);
    QSqlDatabase getConnection(const QString &connection_name);
    void removeConnection(const QString &connection_name);

    // Singleton getter.
    static DatabaseFactory *getInstance();
};

#endif // DATABASEFACTORY_H
