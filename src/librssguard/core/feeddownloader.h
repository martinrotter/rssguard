// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include "core/message.h"
#include "exceptions/applicationexception.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"

#include <QFutureWatcher>
#include <QHash>
#include <QObject>
#include <QPair>

class MessageFilter;

struct FeedUpdateRequest {
    Feed* feed = nullptr;
    ServiceRoot* account = nullptr;
    QHash<ServiceRoot::BagOfMessages, QStringList> stated_messages;
    QHash<QString, QStringList> tagged_messages;
};

// Represents results of batch feed updates.
class FeedDownloadResults {
  public:
    QSet<ServiceRoot*> updatedAccounts() const;
    QHash<Feed*, QString> erroredFeeds() const;
    QHash<Feed*, QList<Message>> updatedFeeds() const;
    QString overview(int how_many_feeds) const;

    void appendUpdatedFeed(Feed* feed, const QList<Message>& updated_unread_msgs);
    void appendErroredFeed(Feed* feed, const QString& error);
    void clear();

    QList<FeedUpdateRequest> feedRequests() const;
    void setFeedRequests(const QList<FeedUpdateRequest>& req);

  private:
    QHash<Feed*, QList<Message>> m_updatedFeeds;
    QHash<Feed*, QString> m_erroredFeeds;
    QSet<ServiceRoot*> m_updatedAccounts;
    QList<FeedUpdateRequest> m_feedRequests;
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
    void clearFeedOverload(Feed* feed);
    bool checkIfFeedOverloaded(Feed* feed) const;

    void skipFeedUpdateWithError(ServiceRoot* acc, Feed* feed, const ApplicationException& ex);
    void updateOneFeed(ServiceRoot* acc,
                       Feed* feed,
                       const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                       const QHash<QString, QStringList>& tagged_messages);
    void finalizeUpdate();
    void removeDuplicateMessages(QList<Message>& messages);
    void removeTooOldMessages(Feed* feed, QList<Message>& msgs);

    QList<Feed*> scrambleFeedsWithSameHost(const QList<Feed*>& feeds) const;
    FeedUpdateResult updateThreadedFeed(const FeedUpdateRequest& fd);

  private:
    bool m_isCacheSynchronizationRunning;
    bool m_stopFetching;
    QMutex m_mutexDb;
    QHash<ServiceRoot*, ApplicationException> m_erroredAccounts;
    QList<FeedUpdateRequest> m_feeds = {};
    QFutureWatcher<FeedUpdateResult> m_watcherLookup;
    FeedDownloadResults m_results;
    QHash<QString, QDateTime> m_overloadedHosts;
};

#endif // FEEDDOWNLOADER_H
