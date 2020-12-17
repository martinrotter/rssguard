// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include <QObject>

#include <QPair>

#include "core/message.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"

class MessageFilter;
class QMutex;

// Represents results of batch feed updates.
class FeedDownloadResults {
  public:
    QList<QPair<QString, int>> updatedFeeds() const;
    QString overview(int how_many_feeds) const;

    void appendUpdatedFeed(const QPair<QString, int>& feed);
    void sort();
    void clear();

  private:

    // QString represents title if the feed, int represents count of newly downloaded messages.
    QList<QPair<QString, int>> m_updatedFeeds;
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
    void synchronizeAccountCaches(const QList<CacheForServiceRoot*>& caches);
    void updateFeeds(const QList<Feed*>& feeds);
    void stopRunningUpdate();

  signals:
    void cachesSynchronized();
    void updateStarted();
    void updateFinished(FeedDownloadResults updated_feeds);
    void updateProgress(const Feed* feed, int current, int total);

  private:
    void updateOneFeed(Feed* feed);
    void updateAvailableFeeds();
    void finalizeUpdate();

    bool m_isCacheSynchronizationRunning;
    bool m_stopCacheSynchronization;
    QList<Feed*> m_feeds = {};
    QMutex* m_mutex;
    FeedDownloadResults m_results;
    int m_feedsUpdated;
    int m_feedsOriginalCount;
};

#endif // FEEDDOWNLOADER_H
