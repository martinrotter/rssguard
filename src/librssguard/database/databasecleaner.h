// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASECLEANER_H
#define DATABASECLEANER_H

#include "definitions/definitions.h"

#include <QObject>
#include <QSqlDatabase>

struct CleanerOrders {
    bool m_removeReadMessages = false;
    bool m_shrinkDatabase = true;
    bool m_removeOldMessages = true;
    bool m_removeRecycleBin = true;
    bool m_removeStarredMessages = false;
    int m_barrierForRemovingOldMessagesInDays = DEFAULT_DAYS_TO_DELETE_MSG;
};

class RSSGUARD_DLLSPEC DatabaseCleaner : public QObject {
    Q_OBJECT

  public:
    enum class Timing {
      Never,
      OnStartup,
      OnExit
    };

    explicit DatabaseCleaner(QObject* parent = nullptr);
    virtual ~DatabaseCleaner() = default;

    static CleanerOrders loadSettings();
    static Timing automaticCleanupTiming();
    static void saveSettings(const CleanerOrders& orders, Timing timing);
    static void runAutomatically(Timing timing);

  signals:
    void purgeStarted();
    void purgeProgress(int progress, const QString& description);
    void purgeFinished();
    void purgeFailed(const QString& error);

  public slots:
    void purgeDatabaseData(CleanerOrders which_data);

  private:
    void purgeStarredMessages();
    void purgeReadMessages();
    void purgeOldMessages(int days);
    void purgeRecycleBin();
};

#endif // DATABASECLEANER_H
