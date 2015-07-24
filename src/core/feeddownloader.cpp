// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/feed.h"
#include "definitions/definitions.h"

#include <QThread>
#include <QDebug>


FeedDownloader::FeedDownloader(QObject *parent) : QObject(parent) {
  qRegisterMetaType<FeedDownloadResults>("FeedDownloadResults");
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

void FeedDownloader::updateFeeds(const QList<Feed*> &feeds) { 
  qDebug().nospace() << "Performing feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  // Job starts now.
  emit started();

  FeedDownloadResults results;

  for (int i = 0, total = feeds.size(); i < total; i++) {
    int updated_messages = feeds.at(i)->update();

    if (updated_messages > 0) {
      results.m_updatedFeeds.append(QPair<QString,int>(feeds.at(i)->title(), updated_messages));
    }

    qDebug("Made progress in feed updates: %d/%d (id of feed is %d).", i + 1, total, feeds.at(i)->id());
    emit progress(feeds.at(i), i + 1, total);
  }

  qDebug().nospace() << "Finished feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit finished(results);
}


QString FeedDownloadResults::getOverview(int how_many_feeds) {
  qSort(m_updatedFeeds.begin(), m_updatedFeeds.end(), FeedDownloadResults::lessThan);

  QStringList result;

  // TODO: Maybe enhance the formatting of this output.
  for (int i = 0, number_items_output = qMin(how_many_feeds, m_updatedFeeds.size()); i < number_items_output; i++) {
    result.append(m_updatedFeeds.at(i).first + QSL(": ") + QString::number(m_updatedFeeds.at(i).second));
  }

  return result.join(QSL("\n"));
}
