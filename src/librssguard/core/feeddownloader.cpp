// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feeddownloader.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/feedfetchexception.h"
#include "exceptions/filteringexception.h"
#include "exceptions/sqlexception.h"
#include "filtering/filteringsystem.h"
#include "filtering/messagefilter.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/thread.h"
#include "network-web/networkfactory.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"
#include "services/abstract/labelsnode.h"

#include <algorithm>

#include <QBitArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QString>
#include <QtConcurrentMap>

FeedDownloader::FeedDownloader()
  : QObject(), m_isUpdateRunning(false), m_isCacheSynchronizationRunning(false), m_stopFetching(false),
    m_watcherLookup(new QFutureWatcher<FeedUpdateResult>(this)) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");

  connect(m_watcherLookup, &QFutureWatcher<FeedUpdateResult>::resultReadyAt, this, [=](int idx) {
    FeedUpdateResult res = m_watcherLookup->resultAt(idx);

    emit updateProgress(res.feed, m_watcherLookup->progressValue(), m_watcherLookup->progressMaximum());
  });
  connect(m_watcherLookup, &QFutureWatcher<FeedUpdateResult>::finished, this, [=]() {
    finalizeUpdate();
  });
}

FeedDownloader::~FeedDownloader() {
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Destroying FeedDownloader instance.";
}

bool FeedDownloader::isUpdateRunning() const {
  return m_isUpdateRunning;
}

void FeedDownloader::synchronizeAccountCaches(const QList<CacheForServiceRoot*>& caches, bool emit_signals) {
  m_isCacheSynchronizationRunning = true;

  for (CacheForServiceRoot* cache : caches) {
    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Synchronizing cache back to server on thread"
             << QUOTE_W_SPACE_DOT(getThreadID());
    cache->saveAllCachedData();

    if (m_stopFetching) {
      qWarningNN << LOGSEC_FEEDDOWNLOADER << "Aborting cache synchronization.";
      break;
    }
  }

  m_isCacheSynchronizationRunning = false;
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "All caches synchronized.";

  if (emit_signals) {
    emit cachesSynchronized();
  }
}

void FeedDownloader::updateFeeds(const QList<Feed*>& feeds) {
  m_isUpdateRunning = !feeds.isEmpty();
  m_erroredAccounts.clear();
  {
    QMutexLocker lck(&m_mutexResults);
    m_results.clear();
  }
  m_feeds.clear();
  m_stopFetching = false;

  if (feeds.isEmpty()) {
    qWarningNN << LOGSEC_FEEDDOWNLOADER << "No feeds to update in worker thread, aborting update.";
    finalizeUpdate();
  }
  else {
    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Starting feed updates from worker in thread"
             << QUOTE_W_SPACE_DOT(getThreadID());

    // Job starts now.
    emit updateStarted();
    QSet<CacheForServiceRoot*> caches;
    QMultiHash<ServiceRoot*, Feed*> feeds_per_root;

    for (auto* fd : feeds) {
      CacheForServiceRoot* fd_cache = fd->account()->toCache();

      if (fd_cache != nullptr) {
        caches.insert(fd_cache);
      }

      feeds_per_root.insert(fd->account(), fd);
    }

    synchronizeAccountCaches(caches.values(), false);

    auto roots = feeds_per_root.uniqueKeys();

    for (auto* rt : std::as_const(roots)) {
      auto fds = scrambleFeedsWithSameHost(feeds_per_root.values(rt));

      QHash<QString, QStringList> per_acc_tags;
      QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>> per_acc_states;

      // Obtain lists of local IDs.
      if (rt->wantsBaggedIdsOfExistingMessages()) {
        // Tags per account.
        per_acc_tags = qApp->database()->worker()->read<QHash<QString, QStringList>>([&](const QSqlDatabase& db) {
          return DatabaseQueries::bagsOfMessages(db, rt->labelsNode()->labels());
        });

        // This account has activated intelligent downloading of messages.
        // Prepare bags.
        for (Feed* fd : std::as_const(fds)) {
          QHash<ServiceRoot::BagOfMessages, QStringList> per_feed_states;

          qApp->database()->worker()->read([&](const QSqlDatabase& db) {
            per_feed_states.insert(ServiceRoot::BagOfMessages::Read,
                                   DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Read, fd));
            per_feed_states.insert(ServiceRoot::BagOfMessages::Unread,
                                   DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Unread, fd));
            per_feed_states.insert(ServiceRoot::BagOfMessages::Starred,
                                   DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Starred, fd));
          });

          per_acc_states.insert(fd->customId(), per_feed_states);

          FeedUpdateRequest fu;

          fu.account = rt;
          fu.feed = fd;
          fu.stated_messages = per_feed_states;
          fu.tagged_messages = per_acc_tags;

          m_feeds.append(fu);
        }
      }
      else {
        for (Feed* fd : std::as_const(fds)) {
          FeedUpdateRequest fu;

          fu.account = rt;
          fu.feed = fd;

          m_feeds.append(fu);
        }
      }

      try {
        rt->aboutToBeginFeedFetching(fds, per_acc_states, per_acc_tags);
      }
      catch (const ApplicationException& ex) {
        // Common error showed, all feeds from the root are errored now!
        m_erroredAccounts.insert(rt, ex);
      }
    }

    std::function<FeedUpdateResult(const FeedUpdateRequest&)> func =
      [=](const FeedUpdateRequest& fd) -> FeedUpdateResult {
#if defined(Q_OS_LINUX)
      setThreadPriority(Priority::Lowest);
#endif
      return updateThreadedFeed(fd);
    };

    m_watcherLookup->setFuture(QtConcurrent::mapped(
#if QT_VERSION_MAJOR > 5
      qApp->workHorsePool(),
#endif
      m_feeds,
      func));
  }
}

void FeedDownloader::clearFeedOverload(Feed* feed) {
  QMutexLocker lck(&m_mutexOverloadedHosts);

  m_overloadedHosts.remove(QUrl(feed->source()).host());
}

bool FeedDownloader::checkIfFeedOverloaded(Feed* feed) const {
  QString hostname = QUrl(feed->source()).host();
  QMutexLocker lck(&m_mutexOverloadedHosts);
  QDateTime retry_after = m_overloadedHosts.value(hostname);

  return retry_after.isValid() && retry_after > QDateTime::currentDateTimeUtc();
}

FeedUpdateResult FeedDownloader::updateThreadedFeed(const FeedUpdateRequest& fd) {
  if (!m_stopFetching) {
    if (m_erroredAccounts.contains(fd.account)) {
      // This feed is errored because its account errored when preparing feed update.
      ApplicationException root_ex = m_erroredAccounts.value(fd.account);

      skipFeedUpdateWithError(fd.account, fd.feed, root_ex);

      QMutexLocker lck_results(&m_mutexResults);
      m_results.appendErroredFeed(fd.feed, root_ex.message());
    }
    else {
      updateOneFeed(fd.account, fd.feed, fd.stated_messages, fd.tagged_messages);
    }

    fd.feed->setLastUpdated(QDateTime::currentDateTimeUtc());
  }

  FeedUpdateResult res;

  res.feed = fd.feed;

  return res;
}

void FeedDownloader::skipFeedUpdateWithError(ServiceRoot* acc, Feed* feed, const ApplicationException& ex) {
  const FeedFetchException* fetch_ex = dynamic_cast<const FeedFetchException*>(&ex);
  const bool update_feed_list =
    qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateFeedListDuringFetching)).toBool();

  if (fetch_ex != nullptr) {
    feed->setStatus(fetch_ex->feedStatus(), fetch_ex->message());
  }
  else {
    feed->setStatus(Feed::Status::OtherError, ex.message());
  }

  if (acc != nullptr && update_feed_list) {
    acc->itemChanged({feed});
  }
}

void FeedDownloader::stopRunningUpdate() {
  m_stopFetching = true;

  if (!m_isUpdateRunning && !m_isCacheSynchronizationRunning) {
    emit stopRequestProcessed();
  }
}

void FeedDownloader::updateOneFeed(ServiceRoot* acc,
                                   Feed* feed,
                                   const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                   const QHash<QString, QStringList>& tagged_messages) {
  // NOTE: This has negative performance impact when fetching bigger number of feeds.
  const bool update_feed_list =
    qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateFeedListDuringFetching)).toBool();

  if (checkIfFeedOverloaded(feed)) {
    qWarningNN << LOGSEC_CORE << "Feed with source" << QUOTE_W_SPACE(feed->source())
               << "was signalled temporarily being down. Returning no articles for now.";

    feed->setStatus(Feed::Status::NetworkError,
                    tr("feed is in network cooldown mode due to making too many network requests"));

    if (update_feed_list) {
      acc->itemChanged({feed});
    }

    return;
  }

  feed->setStatus(Feed::Status::Fetching);

  if (update_feed_list) {
    acc->itemChanged({feed});
  }

  qlonglong thread_id = getThreadID();

  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Downloading new messages for feed ID" << QUOTE_W_SPACE(feed->customId())
           << "URL:" << QUOTE_W_SPACE(feed->source()) << "title:" << QUOTE_W_SPACE(feed->title()) << "in thread "
           << QUOTE_W_SPACE_DOT(thread_id);

  int acc_id = acc->accountId();
  QElapsedTimer tmr;
  tmr.start();

  try {
    QList<Message> msgs = acc->obtainNewMessages(feed, stated_messages, tagged_messages);

    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Downloaded" << NONQUOTE_W_SPACE(msgs.size()) << "messages for feed ID"
             << QUOTE_W_SPACE_COMMA(feed->customId()) << "operation took" << NONQUOTE_W_SPACE(tmr.nsecsElapsed() / 1000)
             << "microseconds.";

    clearFeedOverload(feed);

    if (m_stopFetching) {
      return;
    }

    bool fix_future_datetimes =
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::FixupFutureArticleDateTimes)).toBool();

    // Now, sanitize messages (tweak encoding etc.).
    for (auto& msg : msgs) {
      msg.m_accountId = acc_id;
      msg.m_feedId = feed->id();

      msg.sanitize(feed, fix_future_datetimes);
    }

    QMutexLocker lck(&m_mutexDb);

    if (m_stopFetching) {
      return;
    }

    QList<QPointer<MessageFilter>> feed_filters = feed->messageFilters();
    auto feed_filters_enabled = qlinq::from(feed_filters)
                                  .where([](const QPointer<MessageFilter>& fltr) {
                                    return !fltr.isNull() && fltr->enabled();
                                  })
                                  .orderBy([](const QPointer<MessageFilter>& fltr) {
                                    return fltr->sortOrder();
                                  })
                                  .toList();

    if (!feed_filters_enabled.isEmpty()) {
      tmr.restart();

      // Perform per-message filtering.
      FilteringSystem filtering(FilteringSystem::FiteringUseCase::NewArticles, feed, acc, qApp);
      filtering.filterRun().setTotalCountOfFilters(feed_filters_enabled.size());

      qDebugNN << LOGSEC_FEEDDOWNLOADER << "Setting up JS evaluation took " << tmr.nsecsElapsed() / 1000
               << " microseconds.";

      QList<Message> read_msgs, important_msgs;
      QElapsedTimer tmr_whole;
      QBitArray retained_messages(msgs.size(), true);

      tmr_whole.start();

      for (int i = 0; i < msgs.size(); i++) {
        if (m_stopFetching) {
          return;
        }

        Message msg_original(msgs[i]);
        Message* msg_filtered = &msgs[i];

        // Attach live message object to wrapper.
        tmr.restart();
        filtering.setMessage(msg_filtered);

        bool remove_msg = false;

        for (int j = 0; j < feed_filters_enabled.size(); j++) {
          if (m_stopFetching) {
            return;
          }

          QPointer<MessageFilter> filter = feed_filters_enabled.at(j);

          if (filter.isNull()) {
            qCriticalNN << LOGSEC_FEEDDOWNLOADER
                        << "Article filter was probably deleted, removing its pointer from list of filters.";
            feed_filters_enabled.removeAt(j--);
            continue;
          }

          MessageFilter* msg_filter = filter.data();

          tmr.restart();
          filtering.filterRun().setIndexOfCurrentFilter(j);

          FilterMessage::FilteringAction decision = filtering.filterMessage(*msg_filter);

          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Running filter script, it took " << tmr.nsecsElapsed() / 1000
                   << " microseconds.";

          switch (decision) {
            case FilterMessage::FilteringAction::Accept:
              // Message is normally accepted, it could be tweaked by the filter.
              continue;

            case FilterMessage::FilteringAction::Ignore:
            case FilterMessage::FilteringAction::Purge:
            default:
              // Remove the message, we do not want it.
              remove_msg = true;
              break;
          }

          // If we reach this point, the message will be removed after filtering.
          break;
        }

        if (!remove_msg) {
          filtering.filterRun().incrementNumberOfAcceptedMessages();
        }

        filtering.compareAndWriteArticleStates(&msg_original, msg_filtered, read_msgs, important_msgs);

        if (remove_msg) {
          retained_messages.clearBit(i);
        }
      }

      filtering.setMessage(nullptr);

      int target_index = 0;

      for (int source_index = 0; source_index < msgs.size(); ++source_index) {
        if (retained_messages.testBit(source_index)) {
          if (target_index != source_index) {
            msgs[target_index] = msgs[source_index];
          }

          ++target_index;
        }
      }

      msgs.erase(msgs.begin() + target_index, msgs.end());

      qDebugNN << LOGSEC_CORE << "Filtering flow took" << NONQUOTE_W_SPACE(tmr_whole.elapsed()) << "miliseconds.";

      filtering.pushMessageStatesToServices(read_msgs, important_msgs, feed, acc);
    }

    removeDuplicateMessages(msgs);
    removeTooOldMessages(feed, msgs);

    if (m_stopFetching) {
      return;
    }

    tmr.restart();
    auto updated_messages = acc->updateMessages(msgs, feed, false, update_feed_list);

    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Updating messages in DB took" << NONQUOTE_W_SPACE(tmr.nsecsElapsed() / 1000)
             << "microseconds.";

    if (feed->status() != Feed::Status::NewMessages) {
      feed->setStatus((!updated_messages.m_all.isEmpty() || !updated_messages.m_unread.isEmpty())
                        ? Feed::Status::NewMessages
                        : Feed::Status::Normal);
    }

    qDebugNN << LOGSEC_FEEDDOWNLOADER << updated_messages.m_unread.size() << " unread messages and"
             << NONQUOTE_W_SPACE(updated_messages.m_all.size()) << "total messages for feed"
             << QUOTE_W_SPACE(feed->customId()) << "stored in DB.";

    {
      QMutexLocker lck_results(&m_mutexResults);
      m_results.appendUpdatedFeed(feed, updated_messages.m_unread);
    }
  }
  catch (const FeedFetchException& feed_ex) {
    qCriticalNN << LOGSEC_NETWORK << "Error when fetching feed:" << QUOTE_W_SPACE(feed_ex.feedStatus())
                << "message:" << QUOTE_W_SPACE_DOT(feed_ex.message());

    {
      QMutexLocker lck_results(&m_mutexResults);
      m_results.appendErroredFeed(feed, feed_ex.message());
    }

    feed->setStatus(feed_ex.feedStatus(), feed_ex.message());

    if (feed_ex.feedStatus() == Feed::Status::NetworkError && !feed_ex.data().isNull()) {
      NetworkResult network_result = feed_ex.data().value<NetworkResult>();

      if (network_result.m_httpCode == HTTP_CODE_TOO_MANY_REQUESTS ||
          network_result.m_httpCode == HTTP_CODE_UNAVAILABLE) {
        QDateTime safe_dt = NetworkFactory::extractRetryAfter(network_result.m_headers.value(QSL("retry-after")));

        {
          QMutexLocker lck_hosts(&m_mutexOverloadedHosts);
          m_overloadedHosts.insert(QUrl(feed->source()).host(), safe_dt);
        }

        qDebugNN << LOGSEC_CORE << "Extracted Retry-After value is" << QUOTE_W_SPACE_DOT(safe_dt);
        qWarningNN << LOGSEC_CORE << "Feed" << QUOTE_W_SPACE_DOT(feed->source())
                   << "indicates that there is too many requests right now on the same host.";
      }
    }
  }
  catch (const SqlException& sql_ex) {
    qCriticalNN << LOGSEC_NETWORK << "SQL error when fetching feed:"
                << "message:" << QUOTE_W_SPACE_DOT(sql_ex.message());

    {
      QMutexLocker lck_results(&m_mutexResults);
      m_results.appendErroredFeed(feed, sql_ex.message());
    }
    feed->setStatus(Feed::Status::SqlError, sql_ex.message());
  }
  catch (const FilteringException& filter_ex) {
    qCriticalNN << LOGSEC_NETWORK << "Filtering error when fetching feed" << QUOTE_W_SPACE(feed->title())
                << "and the error is" << QUOTE_W_SPACE_DOT(filter_ex.message());

    {
      QMutexLocker lck_results(&m_mutexResults);
      m_results.appendErroredFeed(feed, filter_ex.message());
    }
    feed->setStatus(Feed::Status::FilteringError, filter_ex.message());
  }
  catch (const ApplicationException& app_ex) {
    qCriticalNN << LOGSEC_NETWORK << "Unknown error when fetching feed" << QUOTE_W_SPACE(feed->title())
                << "and the error is" << QUOTE_W_SPACE_DOT(app_ex.message());

    {
      QMutexLocker lck_results(&m_mutexResults);
      m_results.appendErroredFeed(feed, app_ex.message());
    }
    feed->setStatus(Feed::Status::OtherError, app_ex.message());
  }

  if (update_feed_list) {
    acc->itemChanged({feed});
  }

  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Made progress in feed updates, total feeds count "
           << m_watcherLookup->progressValue() + 1 << "/" << m_feeds.size() << " (id of feed is " << feed->id() << ").";
}

void FeedDownloader::finalizeUpdate() {
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Finished feed updates in thread" << QUOTE_W_SPACE_DOT(getThreadID());

  FeedDownloadResults results;

  {
    QMutexLocker lck(&m_mutexResults);
    m_results.setFeedRequestCount(int(m_feeds.size()));
    results = m_results;
  }

  m_feeds.clear();
  m_isUpdateRunning = false;

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit updateFinished(results);
}

bool FeedDownloader::isCacheSynchronizationRunning() const {
  return m_isCacheSynchronizationRunning;
}

void FeedDownloader::removeDuplicateMessages(QList<Message>& messages) {
  if (messages.size() < 2) {
    return;
  }

  using AnonymousMessageKey = QPair<QString, QPair<QString, QString>>;

  int messages_with_id = 0;
  int messages_with_custom_id = 0;
  int anonymous_messages = 0;

  for (const Message& message : std::as_const(messages)) {
    if (message.m_id > 0) {
      ++messages_with_id;
    }
    else if (!message.m_customId.isEmpty()) {
      ++messages_with_custom_id;
    }
    else {
      ++anonymous_messages;
    }
  }

  QHash<int, int> winners_by_id;
  QHash<QString, int> winners_by_custom_id;
  QHash<AnonymousMessageKey, int> winners_by_attributes;

  winners_by_id.reserve(messages_with_id);
  winners_by_custom_id.reserve(messages_with_custom_id);
  winners_by_attributes.reserve(anonymous_messages);

  auto select_winner = [&messages](auto& winners, const auto& key, int candidate_index) {
    auto winner = winners.find(key);

    if (winner == winners.end()) {
      winners.insert(key, candidate_index);
    }
    else if (messages.at(winner.value()).m_created <= messages.at(candidate_index).m_created) {
      // Keep the latest message, or the last one if creation dates are identical.
      winner.value() = candidate_index;
    }
  };

  for (int i = 0; i < messages.size(); ++i) {
    const Message& message = messages.at(i);

    if (message.m_id > 0) {
      select_winner(winners_by_id, message.m_id, i);
    }
    else if (!message.m_customId.isEmpty()) {
      select_winner(winners_by_custom_id, message.m_customId, i);
    }
    else {
      select_winner(winners_by_attributes,
                    AnonymousMessageKey(message.m_title, qMakePair(message.m_url, message.m_author)),
                    i);
    }
  }

  QBitArray retained_messages(messages.size());
  auto mark_winners = [&retained_messages](const auto& winners) {
    for (auto winner = winners.cbegin(); winner != winners.cend(); ++winner) {
      retained_messages.setBit(winner.value());
    }
  };

  mark_winners(winners_by_id);
  mark_winners(winners_by_custom_id);
  mark_winners(winners_by_attributes);

  int target_index = 0;

  for (int source_index = 0; source_index < messages.size(); ++source_index) {
    if (retained_messages.testBit(source_index)) {
      if (target_index != source_index) {
        messages[target_index] = messages[source_index];
      }

      ++target_index;
    }
    else {
      qWarningNN << LOGSEC_CORE << "Removing article" << QUOTE_W_SPACE(messages.at(source_index).m_title)
                 << "before saving articles to DB, because it is duplicate.";
    }
  }

  messages.erase(messages.begin() + target_index, messages.end());
}

void FeedDownloader::removeTooOldMessages(Feed* feed, QList<Message>& msgs) {
  const Feed::ArticleIgnoreLimit art = feed->articleIgnoreLimit();

  if (!art.m_addAnyArticlesToDb) {
    QDateTime dt_to_avoid;

    if (art.m_dtToAvoid.isValid() && art.m_dtToAvoid.toMSecsSinceEpoch() > 0) {
      dt_to_avoid = art.m_dtToAvoid;
    }
    else if (art.m_hoursToAvoid > 0) {
      dt_to_avoid = QDateTime::currentDateTimeUtc().addSecs((art.m_hoursToAvoid * -3600));
    }
    else if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::AvoidOldArticles)).toBool()) {
      QDateTime global_dt_to_avoid =
        qApp->settings()->value(GROUP(Messages), SETTING(Messages::DateTimeToAvoidArticle)).toDateTime();
      int global_hours_to_avoid =
        qApp->settings()->value(GROUP(Messages), SETTING(Messages::HoursToAvoidArticle)).toInt();

      if (global_dt_to_avoid.isValid() && global_dt_to_avoid.toMSecsSinceEpoch() > 0) {
        dt_to_avoid = global_dt_to_avoid;
      }
      else if (global_hours_to_avoid > 0) {
        dt_to_avoid = QDateTime::currentDateTimeUtc().addSecs(global_hours_to_avoid * -3600);
      }
    }

    if (dt_to_avoid.isValid()) {
      const auto first_removed = std::remove_if(msgs.begin(), msgs.end(), [&dt_to_avoid](const Message& msg) {
        const bool remove = msg.m_createdFromFeed && msg.m_created < dt_to_avoid;

        if (remove) {
          qDebugNN << LOGSEC_CORE << "Removing message" << QUOTE_W_SPACE(msg.m_title) << "for being too old.";
        }

        return remove;
      });

      msgs.erase(first_removed, msgs.end());
    }
  }
}

QList<Feed*> FeedDownloader::scrambleFeedsWithSameHost(const QList<Feed*>& feeds) const {
  if (feeds.size() <= 2) {
    return feeds;
  }

  // Group feeds by host.
  QHash<QString, QList<Feed*>> feeds_by_host;
  feeds_by_host.reserve(feeds.size() / 2); // Reasonable initial capacity.

  for (Feed* feed : feeds) {
    QString source = feed->source();

    // Extract host from URL quickly.
    QString host;
    int schemeEnd = source.indexOf(QSL("://"));
    if (schemeEnd != -1) {
      int host_start = schemeEnd + 3;
      int host_end = source.indexOf('/', host_start);
      host = (host_end != -1) ? source.mid(host_start, host_end - host_start) : source.mid(host_start);
    }
    else {
      // No scheme, treat entire source as host.
      int host_end = source.indexOf(QL1C('/'));
      host = (host_end != -1) ? source.left(host_end) : source;
    }

    feeds_by_host[host].append(feed);
  }

  // If all feeds are from the same host or all different hosts, return original.
  if (feeds_by_host.size() == 1 || feeds_by_host.size() == feeds.size()) {
    return feeds;
  }

  // Distribute feeds evenly using round-robin.
  QList<Feed*> result;
  result.reserve(feeds.size());

  // Create list of iterators for each host group.
  QList<QPair<QString, int>> host_indices;
  host_indices.reserve(feeds_by_host.size());

  for (auto it = feeds_by_host.constBegin(); it != feeds_by_host.constEnd(); ++it) {
    host_indices.append({it.key(), 0});
  }

  // Round-robin distribution.
  int total_added = 0;

  while (total_added < feeds.size()) {
    for (auto& pair : host_indices) {
      const QList<Feed*>& host_feeds = feeds_by_host[pair.first];

      if (pair.second < host_feeds.size()) {
        result.append(host_feeds[pair.second]);
        pair.second++;
        total_added++;
      }
    }
  }

  return result;
}

QString FeedDownloadResults::overview(int how_many_feeds) const {
  QStringList result;
  const int number_items_output = qMin(how_many_feeds, m_updatedFeeds.size());

  int i = 0;

  for (auto it = m_updatedFeeds.cbegin(); it != m_updatedFeeds.cend() && i < number_items_output; ++it, ++i) {
    Feed* fd = it.key();
    const QList<Message>& msgs = it.value();

    if (fd->isQuiet()) {
      continue;
    }

#if defined(Q_OS_LINUX)
    result.append(fd->title().toHtmlEscaped() + QSL(": ") + QString::number(msgs.size()));
#else
    result.append(fd->title() + QSL(": ") + QString::number(msgs.size()));
#endif
  }

  QString res_str = result.join(QSL("\n"));

  if (m_updatedFeeds.size() > how_many_feeds) {
    res_str += QObject::tr("\n\n+ %n other feeds.", nullptr, m_updatedFeeds.size() - how_many_feeds);
  }

  return res_str;
}

void FeedDownloadResults::appendUpdatedFeed(Feed* feed, const QList<Message>& updated_unread_msgs) {
  if (!updated_unread_msgs.isEmpty()) {
    m_updatedFeeds.insert(feed, updated_unread_msgs);
  }

  m_updatedAccounts.insert(feed->account());
}

void FeedDownloadResults::appendErroredFeed(Feed* feed, const QString& error) {
  m_erroredFeeds.insert(feed, error);
}

void FeedDownloadResults::clear() {
  m_updatedFeeds.clear();
  m_erroredFeeds.clear();
  m_updatedAccounts.clear();
  m_feedRequestCount = 0;
}

int FeedDownloadResults::feedRequestCount() const {
  return m_feedRequestCount;
}

void FeedDownloadResults::setFeedRequestCount(int count) {
  m_feedRequestCount = count;
}

const QSet<ServiceRoot*>& FeedDownloadResults::updatedAccounts() const {
  return m_updatedAccounts;
}

const QHash<Feed*, QString>& FeedDownloadResults::erroredFeeds() const {
  return m_erroredFeeds;
}

const QHash<Feed*, QList<Message>>& FeedDownloadResults::updatedFeeds() const {
  return m_updatedFeeds;
}
