// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDREADER_H
#define FEEDREADER_H

#include <QObject>

#include "core/feeddownloader.h"
#include "core/messagefilter.h"
#include "services/abstract/feed.h"

#include <QFutureWatcher>

class FeedsModel;
class MessagesModel;
class MessagesProxyModel;
class FeedsProxyModel;
class ServiceEntryPoint;
class QTimer;
class QThread;

class RSSGUARD_DLLSPEC FeedReader : public QObject {
  Q_OBJECT

  public:
    explicit FeedReader(QObject* parent = nullptr);
    virtual ~FeedReader();

    // List of all installed "feed service plugins".
    QList<ServiceEntryPoint*> feedServices();

    // Access to DB cleaner.
    FeedDownloader* feedDownloader() const;
    FeedsModel* feedsModel() const;
    MessagesModel* messagesModel() const;
    FeedsProxyModel* feedsProxyModel() const;
    MessagesProxyModel* messagesProxyModel() const;

    // Schedules given feeds for update.
    void updateFeeds(const QList<Feed*>& feeds);

    void showMessageFiltersManager();

    // True if feed update is running right now.
    bool isFeedUpdateRunning() const;

    // Resets global auto-update intervals according to settings
    // and starts/stop the timer as needed.
    void updateAutoUpdateStatus();

    bool autoUpdateEnabled() const;
    int autoUpdateRemainingInterval() const;
    int autoUpdateInitialInterval() const;

    void loadSavedMessageFilters();
    QList<MessageFilter*> messageFilters() const;
    MessageFilter* addMessageFilter(const QString& title, const QString& script);
    void removeMessageFilter(MessageFilter* filter);
    void updateMessageFilter(MessageFilter* filter);
    void assignMessageFilterToFeed(Feed* feed, MessageFilter* filter);
    void removeMessageFilterToFeedAssignment(Feed* feed, MessageFilter* filter);

  public slots:
    void updateAllFeeds();
    void updateManuallyIntervaledFeeds();
    void stopRunningFeedUpdate();
    void quit();

  private slots:
    void executeNextAutoUpdate();
    void checkServicesForAsyncOperations();
    void asyncCacheSaveFinished();

  signals:
    void feedUpdatesStarted();
    void feedUpdatesFinished(FeedDownloadResults updated_feeds);
    void feedUpdatesProgress(const Feed* feed, int current, int total);

  private:
    QList<ServiceEntryPoint*> m_feedServices;
    QList<MessageFilter*> m_messageFilters;
    FeedsModel* m_feedsModel;
    FeedsProxyModel* m_feedsProxyModel;
    MessagesModel* m_messagesModel;
    MessagesProxyModel* m_messagesProxyModel;

    // Auto-update stuff.
    QTimer* m_autoUpdateTimer;
    bool m_globalAutoUpdateEnabled{};
    bool m_globalAutoUpdateOnlyUnfocused{};
    int m_globalAutoUpdateInitialInterval{};
    int m_globalAutoUpdateRemainingInterval{};
    QThread* m_feedDownloaderThread;
    FeedDownloader* m_feedDownloader;
};

#endif // FEEDREADER_H
