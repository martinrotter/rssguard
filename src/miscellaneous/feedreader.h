// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FEEDREADER_H
#define FEEDREADER_H

#include <QObject>

#include "services/abstract/feed.h"
#include "core/feeddownloader.h"

#include <QFutureWatcher>


class FeedsModel;
class MessagesModel;
class MessagesProxyModel;
class FeedsProxyModel;
class ServiceEntryPoint;
class ServiceOperator;
class DatabaseCleaner;
class QTimer;

class FeedReader : public QObject {
    Q_OBJECT

  public:
    explicit FeedReader(QObject *parent = 0);
    virtual ~FeedReader();

    // List of all installed "feed service plugins", including obligatory
    // "standard" service entry point.
    QList<ServiceEntryPoint*> feedServices();

    // Access to DB cleaner.
    DatabaseCleaner *databaseCleaner();

    FeedDownloader *feedDownloader() const;
    FeedsModel *feedsModel() const;
    MessagesModel *messagesModel() const;
    FeedsProxyModel *feedsProxyModel() const;
    MessagesProxyModel *messagesProxyModel() const;

    // Schedules given feeds for update.
    void updateFeeds(const QList<Feed*> &feeds);

    // True if feed update is running right now.
    bool isFeedUpdateRunning() const;

    // Resets global auto-update intervals according to settings
    // and starts/stop the timer as needed.
    void updateAutoUpdateStatus();

    bool autoUpdateEnabled() const;
    int autoUpdateRemainingInterval() const;
    int autoUpdateInitialInterval() const;

  public slots:   
    // Schedules all feeds from all accounts for update.
    void updateAllFeeds();
    void stopRunningFeedUpdate();
    void quit();

  private slots:
    // Is executed when next auto-update round could be done.
    void executeNextAutoUpdate();
    void checkServicesForAsyncOperations();
    void checkServicesForAsyncOperations(bool wait_for_future);
    void asyncCacheSaveFinished();

  signals:
    void feedUpdatesStarted();
    void feedUpdatesFinished(FeedDownloadResults updated_feeds);
    void feedUpdatesProgress(const Feed *feed, int current, int total);

  private:
    QList<ServiceEntryPoint*> m_feedServices;

    FeedsModel *m_feedsModel;
    FeedsProxyModel *m_feedsProxyModel;
    MessagesModel *m_messagesModel;
    MessagesProxyModel *m_messagesProxyModel;

    QFutureWatcher<void> *m_cacheSaveFutureWatcher;

    // Auto-update stuff.
    QTimer *m_autoUpdateTimer;
    bool m_globalAutoUpdateEnabled;
    int m_globalAutoUpdateInitialInterval;
    int m_globalAutoUpdateRemainingInterval;

    ServiceOperator *m_serviceOperator;

    QThread *m_feedDownloaderThread;
    FeedDownloader *m_feedDownloader;

    QThread *m_dbCleanerThread;
    DatabaseCleaner *m_dbCleaner;
};

#endif // FEEDREADER_H
