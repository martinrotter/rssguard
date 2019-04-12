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

    // Constructors.
    explicit DatabaseCleaner(QObject* parent = nullptr);
    virtual ~DatabaseCleaner() = default;

  signals:
    void purgeStarted();
    void purgeProgress(int progress, const QString& description);
    void purgeFinished(bool result);

  public slots:
    void purgeDatabaseData(const CleanerOrders& which_data);

  private:
    bool purgeStarredMessages(const QSqlDatabase& database);
    bool purgeReadMessages(const QSqlDatabase& database);
    bool purgeOldMessages(const QSqlDatabase& database, int days);
    bool purgeRecycleBin(const QSqlDatabase& database);
};

#endif // DATABASECLEANER_H
