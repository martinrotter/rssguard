// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feeddownloader.h"

#include "definitions/definitions.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"

#include <QDebug>
#include <QMessageBox>
#include <QMessageLogger>
#include <QMutexLocker>
#include <QString>
#include <QThread>
#include <QThreadPool>

FeedDownloader::FeedDownloader(QObject* parent)
  : QObject(parent), m_mutex(new QMutex()), m_threadPool(new QThreadPool(this)),
  m_feedsUpdated(0), m_feedsUpdating(0), m_feedsOriginalCount(0) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");
  m_threadPool->setMaxThreadCount(2);
}

FeedDownloader::~FeedDownloader() {
  m_mutex->tryLock();
  m_mutex->unlock();
  delete m_mutex;
  qDebug("Destroying FeedDownloader instance.");
}

bool FeedDownloader::isUpdateRunning() const {
  return !m_feeds.isEmpty() || m_feedsUpdating > 0;
}

void FeedDownloader::updateAvailableFeeds() {
  foreach (const Feed* feed, m_feeds) {
    auto* cache = dynamic_cast<CacheForServiceRoot*>(feed->getParentServiceRoot());

    if (cache != nullptr) {
      qDebug("Saving cache for feed with DB ID %d and title '%s'.", feed->id(), qPrintable(feed->title()));
      cache->saveAllCachedData(false);
    }
  }

  while (!m_feeds.isEmpty()) {
    connect(m_feeds.first(), &Feed::messagesObtained, this, &FeedDownloader::oneFeedUpdateFinished,
            (Qt::ConnectionType)(Qt::UniqueConnection | Qt::AutoConnection));

    if (m_threadPool->tryStart(m_feeds.first())) {
      m_feeds.removeFirst();
      m_feedsUpdating++;
    }
    else {
      qCritical("User wanted to update some feeds but all working threads are occupied.");

      // We want to start update of some feeds but all working threads are occupied.
      break;
    }
  }
}

void FeedDownloader::updateFeeds(const QList<Feed*>& feeds) {
  QMutexLocker locker(m_mutex);

  if (feeds.isEmpty()) {
    qDebug("No feeds to update in worker thread, aborting update.");
    finalizeUpdate();
  }
  else {
    qDebug().nospace() << "Starting feed updates from worker in thread: \'" << QThread::currentThreadId() << "\'.";
    m_feeds = feeds;
    m_feedsOriginalCount = m_feeds.size();
    m_results.clear();
    m_feedsUpdated = m_feedsUpdating = 0;

    // Job starts now.
    emit updateStarted();

    updateAvailableFeeds();
  }
}

void FeedDownloader::stopRunningUpdate() {
  m_threadPool->clear();
  m_feeds.clear();
}

void FeedDownloader::oneFeedUpdateFinished(const QList<Message>& messages, bool error_during_obtaining) {
  QMutexLocker locker(m_mutex);

  m_feedsUpdated++;
  m_feedsUpdating--;
  Feed* feed = qobject_cast<Feed*>(sender());

  disconnect(feed, &Feed::messagesObtained, this, &FeedDownloader::oneFeedUpdateFinished);

  // Now, we check if there are any feeds we would like to update too.
  updateAvailableFeeds();

  // Now make sure, that messages are actually stored to SQL in a locked state.
  qDebug().nospace() << "Saving messages of feed ID "
                     << feed->customId() << " URL: " << feed->url() << " title: " << feed->title() << " in thread: \'"
                     << QThread::currentThreadId() << "\'.";

  int updated_messages = feed->updateMessages(messages, error_during_obtaining);

  qDebug("%d messages for feed %s stored in DB.", updated_messages, qPrintable(feed->customId()));

  if (updated_messages > 0) {
    m_results.appendUpdatedFeed(QPair<QString, int>(feed->title(), updated_messages));
  }

  qDebug("Made progress in feed updates, total feeds count %d/%d (id of feed is %d).", m_feedsUpdated, m_feedsOriginalCount, feed->id());
  emit updateProgress(feed, m_feedsUpdated, m_feedsOriginalCount);

  if (m_feeds.isEmpty() && m_feedsUpdating <= 0) {
    finalizeUpdate();
  }
}

void FeedDownloader::finalizeUpdate() {
  qDebug().nospace() << "Finished feed updates in thread: \'" << QThread::currentThreadId() << "\'.";
  m_results.sort();

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit updateFinished(m_results);
}

QString FeedDownloadResults::overview(int how_many_feeds) const {
  QStringList result;

  for (int i = 0, number_items_output = qMin(how_many_feeds, m_updatedFeeds.size()); i < number_items_output; i++) {
    result.append(m_updatedFeeds.at(i).first + QSL(": ") + QString::number(m_updatedFeeds.at(i).second));
  }

  QString res_str = result.join(QSL("\n"));

  if (m_updatedFeeds.size() > how_many_feeds) {
    res_str += QObject::tr("\n\n+ %n other feeds.", nullptr, m_updatedFeeds.size() - how_many_feeds);
  }

  return res_str;
}

void FeedDownloadResults::appendUpdatedFeed(const QPair<QString, int>& feed) {
  m_updatedFeeds.append(feed);
}

void FeedDownloadResults::sort() {
  qSort(m_updatedFeeds.begin(), m_updatedFeeds.end(), FeedDownloadResults::lessThan);
}

bool FeedDownloadResults::lessThan(const QPair<QString, int>& lhs, const QPair<QString, int>& rhs) {
  return lhs.second > rhs.second;
}

void FeedDownloadResults::clear() {
  m_updatedFeeds.clear();
}

QList<QPair<QString, int>> FeedDownloadResults::updatedFeeds() const {
  return m_updatedFeeds;
}
