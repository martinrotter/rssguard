// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASECLEANER_H
#define DATABASECLEANER_H

#include <QObject>
#include <QSqlDatabase>

struct CleanerOrders {
    bool m_removeReadMessages;
    bool m_shrinkDatabase;
    bool m_removeOldMessages;
    bool m_removeRecycleBin;
    bool m_removeStarredMessages;
    int m_barrierForRemovingOldMessagesInDays;
};

class DatabaseCleaner : public QObject {
    Q_OBJECT

  public:
    explicit DatabaseCleaner(QObject* parent = nullptr);
    virtual ~DatabaseCleaner() = default;

  signals:
    void purgeStarted();
    void purgeProgress(int progress, const QString& description);
    void purgeFinished();

  public slots:
    void purgeDatabaseData(CleanerOrders which_data);

  private:
    void purgeStarredMessages(const QSqlDatabase& database);
    void purgeReadMessages(const QSqlDatabase& database);
    void purgeOldMessages(const QSqlDatabase& database, int days);
    void purgeRecycleBin(const QSqlDatabase& database);
};

#endif // DATABASECLEANER_H
