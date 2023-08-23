// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include <QObject>

#include <QFutureWatcher>
#include <QPair>

#include "core/message.h"
#include "exceptions/applicationexception.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"

class MessageFilter;

// Represents results of batch feed updates.
class FeedDownloadResults {
  public:
    QList<QPair<Feed*, int>> updatedFeeds() const;
    QString overview(int how_many_feeds) const;

    void appendUpdatedFeed(const QPair<Feed*, int>& feed);
    void sort();
    void clear();

  private:
    // QString represents title if the feed, int represents count of newly downloaded messages.
    QList<QPair<Feed*, int>> m_updatedFeeds;
};

struct FeedUpdateRequest {
    Feed* feed = nullptr;
    ServiceRoot* account = nullptr;
    QHash<ServiceRoot::BagOfMessages, QStringList> stated_messages;
    QHash<QString, QStringList> tagged_messages;
};

struct FeedUpdateResult {
    Feed* feed = nullptr;
};

// This class offers means to "update" feeds and "special" categories.
// NOTE: This class is used within separate thread.
class FeedDownloader : public QObject {
    Q_OBJECT

  public:
    explicit FeedDownloader();
    virtual ~FeedDownloader();

    bool isUpdateRunning() const;
    bool isCacheSynchronizationRunning() const;

  public slots:
    void synchronizeAccountCaches(const QList<CacheForServiceRoot*>& caches, bool emit_signals);
    void updateFeeds(const QList<Feed*>& feeds);
    void stopRunningUpdate();

  signals:
    void cachesSynchronized();
    void updateStarted();
    void updateFinished(FeedDownloadResults updated_feeds);
    void updateProgress(const Feed* feed, int current, int total);

  private:
    void skipFeedUpdateWithError(ServiceRoot* acc, Feed* feed, const ApplicationException& ex);
    void updateOneFeed(ServiceRoot* acc,
                       Feed* feed,
                       const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                       const QHash<QString, QStringList>& tagged_messages);
    void finalizeUpdate();
    void removeDuplicateMessages(QList<Message>& messages);

    FeedUpdateResult updateThreadedFeed(const FeedUpdateRequest& fd);

  private:
    bool m_isCacheSynchronizationRunning;
    bool m_stopCacheSynchronization;
    QMutex m_mutexDb;
    QHash<ServiceRoot*, ApplicationException> m_erroredAccounts;
    QList<FeedUpdateRequest> m_feeds = {};
    QFutureWatcher<FeedUpdateResult> m_watcherLookup;
    FeedDownloadResults m_results;
};

#endif // FEEDDOWNLOADER_H
