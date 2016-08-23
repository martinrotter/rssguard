// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "core/feeddownloader.h"

#include "services/abstract/feed.h"
#include "definitions/definitions.h"

#include <QThread>
#include <QDebug>
#include <QThreadPool>


FeedDownloader::FeedDownloader(QObject *parent)
  : QObject(parent), m_threadPool(new QThreadPool(this)), m_results(FeedDownloadResults()), m_feedsUpdated(0), m_feedsToUpdate(0),
    m_feedsUpdating(0), m_feedsTotalCount(0), m_stopUpdate(false) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");

  m_threadPool->setMaxThreadCount(FEED_DOWNLOADER_MAX_THREADS);
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

bool FeedDownloader::isUpdateRunning() const {
  return m_feedsToUpdate > 0 || m_feedsUpdating > 0;
}

void FeedDownloader::updateFeeds(const QList<Feed*> &feeds) {
  if (feeds.isEmpty()) {
    qDebug("No feeds to update in worker thread, aborting update.");
    finalizeUpdate();
    return;
  }

  qDebug().nospace() << "Starting feed updates from worker in thread: \'" << QThread::currentThreadId() << "\'.";

  m_results.clear();

  m_feedsUpdated = 0;
  m_feedsUpdating = 0;
  m_feedsToUpdate = feeds.size();
  m_feedsTotalCount = m_feedsToUpdate;

  // Job starts now.
  emit updateStarted();

  for (int i = 0; i < m_feedsTotalCount; i++) {
    connect(feeds.at(i), &Feed::messagesObtained, this, &FeedDownloader::oneFeedUpdateFinished,
            (Qt::ConnectionType) (Qt::UniqueConnection | Qt::AutoConnection));
    m_threadPool->start(feeds.at(i));

    m_feedsUpdating++;
    m_feedsToUpdate--;
  }
}

void FeedDownloader::stopRunningUpdate() {
  m_threadPool->clear();
}

void FeedDownloader::oneFeedUpdateFinished(const QList<Message> &messages) {
  Feed *feed = qobject_cast<Feed*>(sender());

  disconnect(feed, &Feed::messagesObtained, this, &FeedDownloader::oneFeedUpdateFinished);

  m_feedsUpdated++;
  m_feedsUpdating--;

  // Now make sure, that messages are actually stored to SQL in a locked state.
  qDebug().nospace() << "Saving messages of feed "
                     << feed->id() << " in thread: \'"
                     << QThread::currentThreadId() << "\'.";

  int updated_messages;

  QMetaObject::invokeMethod(feed, "updateMessages", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(int, updated_messages),
                            Q_ARG(QList<Message>, messages));

  if (updated_messages > 0) {
    m_results.appendUpdatedFeed(QPair<QString,int>(feed->title(), updated_messages));
  }

  qDebug("Made progress in feed updates, total feeds count %d/%d (id of feed is %d).", m_feedsUpdated, m_feedsTotalCount, feed->id());
  emit updateProgress(feed, m_feedsUpdated, m_feedsTotalCount);

  if (m_threadPool->activeThreadCount() <= 0 || (m_feedsToUpdate <= 0 && m_feedsUpdating <= 0)) {
    finalizeUpdate();
  }
}

void FeedDownloader::finalizeUpdate() {
  qDebug().nospace() << "Finished feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  m_feedsToUpdate = 0;
  m_feedsUpdating = 0;

  m_results.sort();

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit updateFinished(m_results);
}

FeedDownloadResults::FeedDownloadResults() : m_updatedFeeds(QList<QPair<QString,int> >()) {
}

QString FeedDownloadResults::overview(int how_many_feeds) const {
  QStringList result;

  for (int i = 0, number_items_output = qMin(how_many_feeds, m_updatedFeeds.size()); i < number_items_output; i++) {
    result.append(m_updatedFeeds.at(i).first + QSL(": ") + QString::number(m_updatedFeeds.at(i).second));
  }

  QString res_str = result.join(QSL("\n"));

  if (m_updatedFeeds.size() > how_many_feeds) {
    res_str += QObject::tr("\n\n+ %n other feeds.", 0, m_updatedFeeds.size() - how_many_feeds);
  }

  return res_str;
}

void FeedDownloadResults::appendUpdatedFeed(const QPair<QString,int> &feed) {
  m_updatedFeeds.append(feed);
}

void FeedDownloadResults::sort() {
  qSort(m_updatedFeeds.begin(), m_updatedFeeds.end(), FeedDownloadResults::lessThan);
}

bool FeedDownloadResults::lessThan(const QPair<QString, int> &lhs, const QPair<QString, int> &rhs) {
  return lhs.second > rhs.second;
}

void FeedDownloadResults::clear() {
  m_updatedFeeds.clear();
}

QList<QPair<QString,int> > FeedDownloadResults::updatedFeeds() const {
  return m_updatedFeeds;
}
