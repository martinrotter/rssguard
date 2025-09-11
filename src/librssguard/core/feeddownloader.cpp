// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feeddownloader.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/feedfetchexception.h"
#include "exceptions/filteringexception.h"
#include "filtering/filteringsystem.h"
#include "filtering/messagefilter.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/thread.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"
#include "services/abstract/labelsnode.h"

#include <QDebug>
#include <QString>
#include <QtConcurrentMap>

FeedDownloader::FeedDownloader()
  : QObject(), m_isCacheSynchronizationRunning(false), m_stopCacheSynchronization(false) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");

  connect(&m_watcherLookup, &QFutureWatcher<FeedUpdateResult>::resultReadyAt, this, [=](int idx) {
    FeedUpdateResult res = m_watcherLookup.resultAt(idx);

    emit updateProgress(res.feed, m_watcherLookup.progressValue(), m_watcherLookup.progressMaximum());
  });
  connect(&m_watcherLookup, &QFutureWatcher<FeedUpdateResult>::finished, this, [=]() {
    finalizeUpdate();
  });
}

FeedDownloader::~FeedDownloader() {
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Destroying FeedDownloader instance.";
}

bool FeedDownloader::isUpdateRunning() const {
  return !m_feeds.isEmpty();
}

void FeedDownloader::synchronizeAccountCaches(const QList<CacheForServiceRoot*>& caches, bool emit_signals) {
  m_isCacheSynchronizationRunning = true;

  for (CacheForServiceRoot* cache : caches) {
    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Synchronizing cache back to server on thread"
             << QUOTE_W_SPACE_DOT(getThreadID());
    cache->saveAllCachedData(false);

    if (m_stopCacheSynchronization) {
      qWarningNN << LOGSEC_FEEDDOWNLOADER << "Aborting cache synchronization.";

      m_stopCacheSynchronization = false;
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
  m_erroredAccounts.clear();
  m_results.clear();
  m_feeds.clear();

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
      CacheForServiceRoot* fd_cache = fd->getParentServiceRoot()->toCache();

      if (fd_cache != nullptr) {
        caches.insert(fd_cache);
      }

      feeds_per_root.insert(fd->getParentServiceRoot(), fd);
    }

    synchronizeAccountCaches(caches.values(), false);

    auto roots = feeds_per_root.uniqueKeys();
    QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

    for (auto* rt : roots) {
      auto fds = scrambleFeedsWithSameHost(feeds_per_root.values(rt));

      QHash<QString, QStringList> per_acc_tags;
      QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>> per_acc_states;

      // Obtain lists of local IDs.
      if (rt->wantsBaggedIdsOfExistingMessages()) {
        // Tags per account.
        per_acc_tags = DatabaseQueries::bagsOfMessages(database, rt->labelsNode()->labels());

        // This account has activated intelligent downloading of messages.
        // Prepare bags.
        for (Feed* fd : fds) {
          QHash<ServiceRoot::BagOfMessages, QStringList> per_feed_states;

          per_feed_states.insert(ServiceRoot::BagOfMessages::Read,
                                 DatabaseQueries::bagOfMessages(database, ServiceRoot::BagOfMessages::Read, fd));
          per_feed_states.insert(ServiceRoot::BagOfMessages::Unread,
                                 DatabaseQueries::bagOfMessages(database, ServiceRoot::BagOfMessages::Unread, fd));
          per_feed_states.insert(ServiceRoot::BagOfMessages::Starred,
                                 DatabaseQueries::bagOfMessages(database, ServiceRoot::BagOfMessages::Starred, fd));

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
        for (Feed* fd : fds) {
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

    m_watcherLookup.setFuture(QtConcurrent::mapped(
#if QT_VERSION_MAJOR > 5
      qApp->workHorsePool(),
#endif
      m_feeds,
      func));
  }
}

void FeedDownloader::clearFeedOverload(Feed* feed) {
  m_overloadedHosts.remove(QUrl(feed->source()).host());
}

bool FeedDownloader::checkIfFeedOverloaded(Feed* feed) const {
  QString hostname = QUrl(feed->source()).host();
  QDateTime retry_after = m_overloadedHosts.value(hostname);

  return retry_after.isValid() && retry_after > QDateTime::currentDateTimeUtc();
}

FeedUpdateResult FeedDownloader::updateThreadedFeed(const FeedUpdateRequest& fd) {
  if (m_erroredAccounts.contains(fd.account)) {
    // This feed is errored because its account errored when preparing feed update.
    ApplicationException root_ex = m_erroredAccounts.value(fd.account);

    skipFeedUpdateWithError(fd.account, fd.feed, root_ex);
  }
  else {
    updateOneFeed(fd.account, fd.feed, fd.stated_messages, fd.tagged_messages);
  }

  fd.feed->setLastUpdated(QDateTime::currentDateTimeUtc());

  FeedUpdateResult res;

  res.feed = fd.feed;

  return res;
}

void FeedDownloader::skipFeedUpdateWithError(ServiceRoot* acc, Feed* feed, const ApplicationException& ex) {
  const FeedFetchException* fetch_ex = dynamic_cast<const FeedFetchException*>(&ex);

  if (fetch_ex != nullptr) {
    feed->setStatus(fetch_ex->feedStatus(), fetch_ex->message());
  }
  else {
    feed->setStatus(Feed::Status::OtherError, ex.message());
  }
}

void FeedDownloader::stopRunningUpdate() {
  m_stopCacheSynchronization = true;

  m_watcherLookup.cancel();
  m_watcherLookup.waitForFinished();

  m_feeds.clear();
}

void FeedDownloader::updateOneFeed(ServiceRoot* acc,
                                   Feed* feed,
                                   const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                   const QHash<QString, QStringList>& tagged_messages) {
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
    QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
    QList<Message> msgs = feed->getParentServiceRoot()->obtainNewMessages(feed, stated_messages, tagged_messages);

    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Downloaded" << NONQUOTE_W_SPACE(msgs.size()) << "messages for feed ID"
             << QUOTE_W_SPACE_COMMA(feed->customId()) << "operation took" << NONQUOTE_W_SPACE(tmr.nsecsElapsed() / 1000)
             << "microseconds.";

    clearFeedOverload(feed);

    bool fix_future_datetimes =
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::FixupFutureArticleDateTimes)).toBool();

    // Now, sanitize messages (tweak encoding etc.).
    for (auto& msg : msgs) {
      msg.m_accountId = acc_id;
      msg.sanitize(feed, fix_future_datetimes);
    }

    QMutexLocker lck(&m_mutexDb);

    if (!feed->messageFilters().isEmpty()) {
      tmr.restart();

      // Perform per-message filtering.
      FilteringSystem filtering(FilteringSystem::FiteringUseCase::NewArticles,
                                database,
                                feed,
                                feed->getParentServiceRoot());
      auto feed_filters = feed->messageFilters();

      filtering.filterRun().setTotalCountOfFilters(feed_filters.size());

      qDebugNN << LOGSEC_FEEDDOWNLOADER << "Setting up JS evaluation took " << tmr.nsecsElapsed() / 1000
               << " microseconds.";

      QList<Message> read_msgs, important_msgs;

      for (int i = 0; i < msgs.size(); i++) {
        Message msg_original(msgs[i]);
        Message* msg_tweaked_by_filter = &msgs[i];

        // Attach live message object to wrapper.
        tmr.restart();
        filtering.setMessage(msg_tweaked_by_filter);

        bool remove_msg = false;

        for (int j = 0; j < feed_filters.size(); j++) {
          QPointer<MessageFilter> filter = feed_filters.at(j);

          if (filter.isNull()) {
            qCriticalNN << LOGSEC_FEEDDOWNLOADER
                        << "Article filter was probably deleted, removing its pointer from list of filters.";
            feed_filters.removeAt(j--);
            continue;
          }

          MessageFilter* msg_filter = filter.data();

          tmr.restart();

          filtering.filterRun().setIndexOfCurrentFilter(j);

          try {
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
          }
          catch (const FilteringException& ex) {
            qCriticalNN << LOGSEC_FEEDDOWNLOADER
                        << "Error when evaluating filtering JS function: " << QUOTE_W_SPACE_DOT(ex.message())
                        << " Accepting message.";
            continue;
          }

          // If we reach this point. Then we ignore the message which is by now
          // already removed, go to next message.
          break;
        }

        // NOTE: Categories are heap-allocated and must be destroyed manually
        // after filtering of that particular message is done.
        msg_original.deallocateCategories();

        if (!remove_msg) {
          filtering.filterRun().incrementNumberOfAcceptedMessages();
        }

        if (!msg_original.m_isRead && msg_tweaked_by_filter->m_isRead) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID:" << QUOTE_W_SPACE(msg_original.m_customId)
                   << "was marked as read by message scripts.";

          read_msgs << *msg_tweaked_by_filter;
        }

        if (!msg_original.m_isImportant && msg_tweaked_by_filter->m_isImportant) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID:" << QUOTE_W_SPACE(msg_original.m_customId)
                   << "was marked as important by message scripts.";

          important_msgs << *msg_tweaked_by_filter;
        }

        // NOTE: We only remember what labels were added/removed in filters
        // and store the fact to server (of synchronized) and local DB later.
        // This is mainly because articles might not even be in DB yet.
        // So first insert articles, then update their label assignments etc.
        for (Label* lbl : std::as_const(msg_original.m_assignedLabels)) {
          if (!msg_tweaked_by_filter->m_assignedLabels.contains(lbl)) {
            // Label is not there anymore, it was deassigned.
            msg_tweaked_by_filter->m_deassignedLabelsByFilter << lbl;
          }
        }

        for (Label* lbl : std::as_const(msg_tweaked_by_filter->m_assignedLabels)) {
          if (!msg_original.m_assignedLabels.contains(lbl)) {
            // Label is in new message, but is not in old message, it
            // was newly assigned.
            msg_tweaked_by_filter->m_assignedLabelsByFilter << lbl;
          }
        }

        if (remove_msg) {
          msgs.removeAt(i--);
        }
      }

      if (!read_msgs.isEmpty()) {
        // Now we push new read states to the service.
        if (feed->getParentServiceRoot()->onBeforeSetMessagesRead(feed, read_msgs, RootItem::ReadStatus::Read)) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Notified services about messages marked as read by message filters.";
        }
        else {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Notification of services about messages marked as read by message filters FAILED.";
        }
      }

      if (!important_msgs.isEmpty()) {
        // Now we push new read states to the service.
        auto list = boolinq::from(important_msgs)
                      .select([](const Message& msg) {
                        return ImportanceChange(msg, RootItem::Importance::Important);
                      })
                      .toStdList();
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

    removeDuplicateMessages(msgs);
    removeTooOldMessages(feed, msgs);

    tmr.restart();
    auto updated_messages = acc->updateMessages(msgs, feed, false, nullptr);

    qDebugNN << LOGSEC_FEEDDOWNLOADER << "Updating messages in DB took" << NONQUOTE_W_SPACE(tmr.nsecsElapsed() / 1000)
             << "microseconds.";

    if (feed->status() != Feed::Status::NewMessages) {
      feed->setStatus((!updated_messages.m_all.isEmpty() || !updated_messages.m_unread.isEmpty())
                        ? Feed::Status::NewMessages
                        : Feed::Status::Normal);
    }

    qDebugNN << LOGSEC_FEEDDOWNLOADER << updated_messages.m_unread.size() << " unread messages and"
             << NONQUOTE_W_SPACE(updated_messages.m_all.size()) "total messages for feed"
             << QUOTE_W_SPACE(feed->customId()) << "stored in DB.";

    m_results.appendUpdatedFeed(feed, updated_messages.m_unread);
  }
  catch (const FeedFetchException& feed_ex) {
    qCriticalNN << LOGSEC_NETWORK << "Error when fetching feed:" << QUOTE_W_SPACE(feed_ex.feedStatus())
                << "message:" << QUOTE_W_SPACE_DOT(feed_ex.message());

    m_results.appendErroredFeed(feed, feed_ex.message());
    feed->setStatus(feed_ex.feedStatus(), feed_ex.message());

    if (feed_ex.feedStatus() == Feed::Status::NetworkError && !feed_ex.data().isNull()) {
      NetworkResult network_result = feed_ex.data().value<NetworkResult>();

      if (network_result.m_httpCode == HTTP_CODE_TOO_MANY_REQUESTS ||
          network_result.m_httpCode == HTTP_CODE_UNAVAILABLE) {
        QDateTime safe_dt = NetworkFactory::extractRetryAfter(network_result.m_headers.value(QSL("retry-after")));

        m_overloadedHosts.insert(QUrl(feed->source()).host(), safe_dt);

        qDebugNN << LOGSEC_CORE << "Extracted Retry-After value is" << QUOTE_W_SPACE_DOT(safe_dt);
        qWarningNN << LOGSEC_CORE << "Feed" << QUOTE_W_SPACE_DOT(feed->source())
                   << "indicates that there is too many requests right now on the same host.";
      }
    }
  }
  catch (const ApplicationException& app_ex) {
    qCriticalNN << LOGSEC_NETWORK << "Unknown error when fetching feed:"
                << "message:" << QUOTE_W_SPACE_DOT(app_ex.message());

    m_results.appendErroredFeed(feed, app_ex.message());
    feed->setStatus(Feed::Status::OtherError, app_ex.message());
  }

  if (update_feed_list) {
    acc->itemChanged({feed});
  }

  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Made progress in feed updates, total feeds count "
           << m_watcherLookup.progressValue() + 1 << "/" << m_feeds.size() << " (id of feed is " << feed->id() << ").";
}

void FeedDownloader::finalizeUpdate() {
  qDebugNN << LOGSEC_FEEDDOWNLOADER << "Finished feed updates in thread" << QUOTE_W_SPACE_DOT(getThreadID());

  m_feeds.clear();

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit updateFinished(m_results);
}

bool FeedDownloader::isCacheSynchronizationRunning() const {
  return m_isCacheSynchronizationRunning;
}

void FeedDownloader::removeDuplicateMessages(QList<Message>& messages) {
  auto idx = 0;

  while (idx < messages.size()) {
    Message& message = messages[idx];
    std::function<bool(const Message& a, const Message& b)> is_duplicate;

    if (message.m_id > 0) {
      is_duplicate = [](const Message& a, const Message& b) {
        return a.m_id == b.m_id;
      };
    }
    else if (message.m_customId.isEmpty()) {
      is_duplicate = [](const Message& a, const Message& b) {
        return std::tie(a.m_title, a.m_url, a.m_author) == std::tie(b.m_title, b.m_url, b.m_author);
      };
    }
    else {
      is_duplicate = [](const Message& a, const Message& b) {
        return a.m_customId == b.m_customId;
      };
    }

    auto next_idx = idx + 1; // Index of next message to check after removing all duplicates.
    auto last_idx = idx;     // Index of the last kept duplicate.

    idx = next_idx;

    // Remove all duplicate messages, and keep the message with the latest created date.
    // If the created date is identical for all duplicate messages then keep the last message in the list.
    while (idx < messages.size()) {
      auto& last_duplicate = messages[last_idx];

      if (is_duplicate(last_duplicate, messages[idx])) {
        if (last_duplicate.m_created <= messages[idx].m_created) {
          // The last seen message was created earlier or at the same date -- keep the current, and remove the last.
          qWarningNN << LOGSEC_CORE << "Removing article" << QUOTE_W_SPACE(last_duplicate.m_title)
                     << "before saving articles to DB, because it is duplicate.";

          messages.removeAt(last_idx);
          if (last_idx + 1 == next_idx) {
            // The `next_idx` was pointing to the message following the duplicate. With that duplicate removed the
            // next index needs to be adjusted.
            next_idx = last_idx;
          }

          last_idx = idx;
          ++idx;
        }
        else {
          qWarningNN << LOGSEC_CORE << "Removing article" << QUOTE_W_SPACE(messages[idx].m_title)
                     << "before saving articles to DB, because it is duplicate.";

          messages.removeAt(idx);
        }
      }
      else {
        ++idx;
      }
    }

    idx = next_idx;
  }
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
      for (int i = 0; i < msgs.size(); i++) {
        const auto& mss = msgs.at(i);

        if (mss.m_createdFromFeed && mss.m_created < dt_to_avoid) {
          qDebugNN << LOGSEC_CORE << "Removing message" << QUOTE_W_SPACE(mss.m_title) << "for being too old.";
          msgs.removeAt(i--);
        }
      }
    }
  }
}

QList<Feed*> FeedDownloader::scrambleFeedsWithSameHost(const QList<Feed*>& feeds) const {
  return feeds;
}

QString FeedDownloadResults::overview(int how_many_feeds) const {
  QStringList result;

  for (int i = 0, number_items_output = qMin(how_many_feeds, m_updatedFeeds.size()); i < number_items_output; i++) {
    auto* fd = m_updatedFeeds.keys().at(i);
    auto msgs = m_updatedFeeds.value(fd);

    if (fd->isQuiet()) {
      continue;
    }

    result.append(fd->title() + QSL(": ") + QString::number(msgs.size()));
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
}

void FeedDownloadResults::appendErroredFeed(Feed* feed, const QString& error) {
  m_erroredFeeds.insert(feed, error);
}

void FeedDownloadResults::clear() {
  m_updatedFeeds.clear();
  m_erroredFeeds.clear();
}

QHash<Feed*, QString> FeedDownloadResults::erroredFeeds() const {
  return m_erroredFeeds;
}

QHash<Feed*, QList<Message>> FeedDownloadResults::updatedFeeds() const {
  return m_updatedFeeds;
}
