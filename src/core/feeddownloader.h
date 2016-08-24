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

#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include <QObject>

#include <QPair>

#include "core/message.h"


class Feed;
class QThreadPool;
class QMutex;

// Represents results of batch feed updates.
class FeedDownloadResults {
  public:
    explicit FeedDownloadResults();

    QList<QPair<QString,int> > updatedFeeds() const;
    QString overview(int how_many_feeds) const;

    void appendUpdatedFeed(const QPair<QString,int> &feed);
    void sort();
    void clear();

    static bool lessThan(const QPair<QString,int> &lhs, const QPair<QString,int> &rhs);

  private:
    // QString represents title if the feed, int represents count of newly downloaded messages.
    QList<QPair<QString,int> > m_updatedFeeds;
};

// This class offers means to "update" feeds and "special" categories.
// NOTE: This class is used within separate thread.
class FeedDownloader : public QObject {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedDownloader(QObject *parent = 0);
    virtual ~FeedDownloader();

    bool isUpdateRunning() const;

  public slots:
    // Performs update of all feeds from the "feeds" parameter.
    // New messages are downloaded for each feed and they
    // are stored persistently in the database.
    // Appropriate signals are emitted.
    void updateFeeds(const QList<Feed*> &feeds);

    // Stops running update.
    void stopRunningUpdate();

  private slots:
    void oneFeedUpdateFinished(const QList<Message> &messages);

  signals:
    // Emitted if feed updates started.
    void updateStarted();

    // Emitted if all items from update queue are
    // processed.
    void updateFinished(FeedDownloadResults updated_feeds);

    // Emitted if any item is processed.
    // "Current" number indicates count of processed feeds
    // and "total" number indicates total number of feeds
    // which were in the initial queue.
    void updateProgress(const Feed *feed, int current, int total);

  private:
    void updateAvailableFeeds();
    void finalizeUpdate();

    QList<Feed*> m_feeds;
    QMutex *m_mutex;
    QThreadPool *m_threadPool;
    FeedDownloadResults m_results;

    int m_feedsUpdated;
    int m_feedsUpdating;
    int m_feedsTotalCount;
};

#endif // FEEDDOWNLOADER_H
