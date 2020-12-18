// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feeddownloader.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/feedsmodel.h"
#include "core/messagefilter.h"
#include "definitions/definitions.h"
#include "exceptions/filteringexception.h"
#include "miscellaneous/application.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"
#include "services/abstract/labelsnode.h"

#include <QDebug>
#include <QJSEngine>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QString>
#include <QThread>
#include <QUrl>

FeedDownloader::FeedDownloader()
  : QObject(), m_isCacheSynchronizationRunning(false), m_stopCacheSynchronization(false), m_mutex(new QMutex()), m_feedsUpdated(0), m_feedsOriginalCount(0) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");
}

FeedDownloader::~FeedDownloader() {
  m_mutex->tryLock();
  m_mutex->unlock();
  delete m_mutex;
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Destroying FeedDownloader instance.";
}

bool FeedDownloader::isUpdateRunning() const {
  return !m_feeds.isEmpty();
}

void FeedDownloader::updateAvailableFeeds() {
  for (const Feed* feed : m_feeds) {
    auto* cache = dynamic_cast<CacheForServiceRoot*>(feed->getParentServiceRoot());

    if (cache != nullptr) {
      qDebugNN << LOGSEC_FEEDDOWNLOADER
               << "Saving cache for feed with DB ID '" << feed->id()
               << "' and title '" << feed->title() << "'.";
      cache->saveAllCachedData();
    }

    if (m_stopCacheSynchronization) {
      qWarningNN << LOGSEC_FEEDDOWNLOADER << "Aborting cache synchronization.";

      m_stopCacheSynchronization = false;
      break;
    }
  }

  while (!m_feeds.isEmpty()) {
    updateOneFeed(m_feeds.takeFirst());
  }
}

void FeedDownloader::synchronizeAccountCaches(const QList<CacheForServiceRoot*>& caches) {
  m_isCacheSynchronizationRunning = true;

  for (CacheForServiceRoot* cache : caches) {
    qDebugNN << LOGSEC_FEEDDOWNLOADER
             << "Synchronizing cache back to server on thread" << QUOTE_W_SPACE_DOT(QThread::currentThreadId());
    cache->saveAllCachedData();

    if (m_stopCacheSynchronization) {
      qWarningNN << LOGSEC_FEEDDOWNLOADER << "Aborting cache synchronization.";

      m_stopCacheSynchronization = false;
      break;
    }
  }

  m_isCacheSynchronizationRunning = false;
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "All caches synchronized.";
  emit cachesSynchronized();
}

void FeedDownloader::updateFeeds(const QList<Feed*>& feeds) {
  QMutexLocker locker(m_mutex);

  if (feeds.isEmpty()) {
    qDebugNN << LOGSEC_FEEDDOWNLOADER << "No feeds to update in worker thread, aborting update.";
  }
  else {
    qDebugNN << LOGSEC_FEEDDOWNLOADER
             << "Starting feed updates from worker in thread: '"
             << QThread::currentThreadId() << "'.";
    m_feeds = feeds;
    m_feedsOriginalCount = m_feeds.size();
    m_results.clear();
    m_feedsUpdated = 0;

    // Job starts now.
    emit updateStarted();

    updateAvailableFeeds();
  }

  finalizeUpdate();
}

void FeedDownloader::stopRunningUpdate() {
  m_stopCacheSynchronization = true;
  m_feeds.clear();
  m_feedsOriginalCount = m_feedsUpdated = 0;
}

void FeedDownloader::updateOneFeed(Feed* feed) {
  qDebugNN << LOGSEC_FEEDDOWNLOADER
           << "Downloading new messages for feed ID '"
           << feed->customId() << "' URL: '" << feed->url() << "' title: '" << feed->title() << "' in thread: '"
           << QThread::currentThreadId() << "'.";

  bool error_during_obtaining = false;
  int acc_id = feed->getParentServiceRoot()->accountId();
  QElapsedTimer tmr; tmr.start();
  QList<Message> msgs = feed->obtainNewMessages(&error_during_obtaining);

  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Downloaded " << msgs.size() << " messages for feed ID '"
           << feed->customId() << "' URL: '" << feed->url() << "' title: '" << feed->title() << "' in thread: '"
           << QThread::currentThreadId() << "'. Operation took " << tmr.nsecsElapsed() / 1000 << " microseconds.";

  // Now, sanitize messages (tweak encoding etc.).
  for (auto& msg : msgs) {
    msg.m_accountId = acc_id;
    msg.sanitize();
  }

  if (!feed->messageFilters().isEmpty()) {
    tmr.restart();

    bool is_main_thread = QThread::currentThread() == qApp->thread();
    QSqlDatabase database = is_main_thread ?
                            qApp->database()->connection(metaObject()->className()) :
                            qApp->database()->connection(QSL("feed_upd"));

    // Perform per-message filtering.
    QJSEngine filter_engine;

    // Create JavaScript communication wrapper for the message.
    MessageObject msg_obj(&database,
                          feed->customId(),
                          feed->getParentServiceRoot()->accountId(),
                          feed->getParentServiceRoot()->labelsNode()->labels());

    MessageFilter::initializeFilteringEngine(filter_engine, &msg_obj);

    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Setting up JS evaluation took " << tmr.nsecsElapsed() / 1000 << " microseconds.";

    QList<Message> read_msgs, important_msgs;

    for (int i = 0; i < msgs.size(); i++) {
      Message msg_backup(msgs[i]);
      Message* msg_orig = &msgs[i];

      // Attach live message object to wrapper.
      tmr.restart();
      msg_obj.setMessage(msg_orig);
      qDebugNN << LOGSEC_FEEDDOWNLOADER << "Hooking message took " << tmr.nsecsElapsed() / 1000 << " microseconds.";

      auto feed_filters = feed->messageFilters();
      bool remove_msg = false;

      for (int j = 0; j < feed_filters.size(); j++) {
        QPointer<MessageFilter> filter = feed_filters.at(j);

        if (filter.isNull()) {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Message filter was probably deleted, removing its pointer from list of filters.";
          feed_filters.removeAt(j--);
          continue;
        }

        MessageFilter* msg_filter = filter.data();

        tmr.restart();

        try {
          MessageObject::FilteringAction decision = msg_filter->filterMessage(&filter_engine);

          qDebugNN << LOGSEC_FEEDDOWNLOADER
                   << "Running filter script, it took " << tmr.nsecsElapsed() / 1000 << " microseconds.";

          switch (decision) {
            case MessageObject::FilteringAction::Accept:
              // Message is normally accepted, it could be tweaked by the filter.
              continue;

            case MessageObject::FilteringAction::Ignore:
            case MessageObject::FilteringAction::Purge:
            default:
              // Remove the message, we do not want it.
              remove_msg = true;
              break;
          }
        }
        catch (const FilteringException& ex) {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Error when evaluating filtering JS function: '"
                      << ex.message()
                      << "'. Accepting message.";
          continue;
        }

        // If we reach this point. Then we ignore the message which is by now
        // already removed, go to next message.
        break;
      }

      if (!msg_backup.m_isRead && msg_orig->m_isRead) {
        qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID: '" << msg_backup.m_customId << "' was marked as read by message scripts.";

        read_msgs << *msg_orig;
      }

      if (!msg_backup.m_isImportant && msg_orig->m_isImportant) {
        qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID: '" << msg_backup.m_customId << "' was marked as important by message scripts.";

        important_msgs << *msg_orig;
      }

      // Process changed labels.
      for (Label* lbl : msg_backup.m_assignedLabels) {
        if (!msg_orig->m_assignedLabels.contains(lbl)) {
          // Label is not there anymore, it was deassigned.
          lbl->deassignFromMessage(*msg_orig);

          qDebugNN << LOGSEC_FEEDDOWNLOADER
                   << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
                   << "was DEASSIGNED from message" << QUOTE_W_SPACE(msg_orig->m_customId)
                   << "by message filter(s).";
        }
      }

      for (Label* lbl : msg_orig->m_assignedLabels) {
        if (!msg_backup.m_assignedLabels.contains(lbl)) {
          // Label is in new message, but is not in old message, it
          // was newly assigned.
          lbl->assignToMessage(*msg_orig);

          qDebugNN << LOGSEC_FEEDDOWNLOADER
                   << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
                   << "was ASSIGNED to message" << QUOTE_W_SPACE(msg_orig->m_customId)
                   << "by message filter(s).";
        }
      }

      if (remove_msg) {
        msgs.removeAt(i--);
      }
    }

    if (!read_msgs.isEmpty()) {
      // Now we push new read states to the service.
      if (feed->getParentServiceRoot()->onBeforeSetMessagesRead(feed, read_msgs, RootItem::ReadStatus::Read)) {
        qDebugNN << LOGSEC_FEEDDOWNLOADER
                 << "Notified services about messages marked as read by message filters.";
      }
      else {
        qCriticalNN << LOGSEC_FEEDDOWNLOADER
                    << "Notification of services about messages marked as read by message filters FAILED.";
      }
    }

    if (!important_msgs.isEmpty()) {
      // Now we push new read states to the service.
      auto list = boolinq::from(important_msgs).select([](const Message& msg) {
        return ImportanceChange(msg, RootItem::Importance::Important);
      }).toStdList();
      QList<ImportanceChange> chngs = FROM_STD_LIST(QList<ImportanceChange>, list);

      if (feed->getParentServiceRoot()->onBeforeSwitchMessageImportance(feed, chngs)) {
        qDebugNN << LOGSEC_FEEDDOWNLOADER
                 << "Notified services about messages marked as important by message filters.";
      }
      else {
        qCriticalNN << LOGSEC_FEEDDOWNLOADER
                    << "Notification of services about messages marked as important by message filters FAILED.";
      }
    }
  }

  m_feedsUpdated++;

  // Now make sure, that messages are actually stored to SQL in a locked state.
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Saving messages of feed ID '"
           << feed->customId() << "' URL: '" << feed->url() << "' title: '" << feed->title() << "' in thread: '"
           << QThread::currentThreadId() << "'.";

  int updated_messages = feed->updateMessages(msgs, error_during_obtaining);

  qDebugNN << LOGSEC_FEEDDOWNLOADER
           << updated_messages << " messages for feed "
           << feed->customId() << " stored in DB.";

  if (updated_messages > 0) {
    m_results.appendUpdatedFeed(QPair<QString, int>(feed->title(), updated_messages));
  }

  qDebugNN << LOGSEC_FEEDDOWNLOADER
           << "Made progress in feed updates, total feeds count "
           << m_feedsUpdated << "/" << m_feedsOriginalCount << " (id of feed is "
           << feed->id() << ").";
  emit updateProgress(feed, m_feedsUpdated, m_feedsOriginalCount);
}

void FeedDownloader::finalizeUpdate() {
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Finished feed updates in thread: '" << QThread::currentThreadId() << "'.";
  m_results.sort();

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit updateFinished(m_results);
}

bool FeedDownloader::isCacheSynchronizationRunning() const
{
  return m_isCacheSynchronizationRunning;
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
  std::sort(m_updatedFeeds.begin(), m_updatedFeeds.end(), [](const QPair<QString, int>& lhs, const QPair<QString, int>& rhs) {
    return lhs.second > rhs.second;
  });
}

void FeedDownloadResults::clear() {
  m_updatedFeeds.clear();
}

QList<QPair<QString, int>> FeedDownloadResults::updatedFeeds() const {
  return m_updatedFeeds;
}
