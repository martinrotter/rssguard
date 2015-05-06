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

#include "core/feedsmodelfeed.h"

#include <QThread>
#include <QDebug>


FeedDownloader::FeedDownloader(QObject *parent) : QObject(parent) {
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

void FeedDownloader::updateFeeds(const QList<FeedsModelFeed*> &feeds) { 
  qDebug().nospace() << "Performing feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  // Job starts now.
  emit started();

  for (int i = 0, total = feeds.size(); i < total; i++) {
    feeds.at(i)->update();
    qDebug("Made progress in feed updates: %d/%d (id of feed is %d).", i + 1, total, feeds.at(i)->id());
    emit progress(feeds.at(i), i + 1, total);
  }

  qDebug().nospace() << "Finished feed updates in thread: \'" << QThread::currentThreadId() << "\'.";

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit finished();
}
