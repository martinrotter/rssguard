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

#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include <QObject>

#include <QPair>


class FeedsModelFeed;

// Represents results of batch feed updates.
struct FeedDownloadResults {
    explicit FeedDownloadResults() : m_updatedFeeds(QList<QPair<QString,int> >()) {
    }

    QString getOverview(int how_many_feeds);

    static bool lessThan(const QPair<QString,int> &lhs, const QPair<QString,int> &rhs) {
      return lhs.second > rhs.second;
    }

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

  public slots:
    // Performs update of all feeds from the "feeds" parameter.
    // New messages are downloaded for each feed and they
    // are stored persistently in the database.
    // Appropriate signals are emitted.
    void updateFeeds(const QList<FeedsModelFeed*> &feeds);

  signals:
    // Emitted if feed updates started.
    void started();

    // Emitted if all items from update queue are
    // processed.
    void finished(FeedDownloadResults updated_feeds);

    // Emitted if any item is processed.
    // "Current" number indicates count of processed feeds
    // and "total" number indicates total number of feeds
    // which were in the initial queue.
    void progress(FeedsModelFeed *feed, int current, int total);
};

#endif // FEEDDOWNLOADER_H
