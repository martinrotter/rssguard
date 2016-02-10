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
#include <QMetaType>


FeedDownloader::FeedDownloader(QObject *parent) : QObject(parent), m_stopUpdate(false), m_isUpdateRunning(false) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

bool FeedDownloader::isUpdateRunning() const {
  return m_isUpdateRunning;
}

void FeedDownloader::updateFeeds(const QList<Feed*> &feeds) {
  qDebug().nospace() << "Performing feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  // It may be good to disable "stop" action when batch feed update
  // starts.
  m_isUpdateRunning = true;
  m_stopUpdate = false;

  // Job starts now.
  emit started();

  FeedDownloadResults results;

  for (int i = 0, total = feeds.size(); i < total; i++) {
    if (m_stopUpdate) {
      qDebug("Stopping batch feed update now.");
      break;
    }

    int updated_messages = feeds.at(i)->update();

    if (updated_messages > 0) {
      results.appendUpdatedFeed(QPair<QString,int>(feeds.at(i)->title(), updated_messages));
    }

    qDebug("Made progress in feed updates: %d/%d (id of feed is %d).", i + 1, total, feeds.at(i)->id());
    emit progress(feeds.at(i), i + 1, total);
  }

  qDebug().nospace() << "Finished feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  results.sort();

  // Make sure that there is not "stop" action pending.
  m_isUpdateRunning = false;
  m_stopUpdate = false;

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit finished(results);
}

void FeedDownloader::stopRunningUpdate() {
  m_stopUpdate = true;
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

QList<QPair<QString,int> > FeedDownloadResults::updatedFeeds() const {
  return m_updatedFeeds;
}
