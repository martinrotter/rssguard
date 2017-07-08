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

#include "miscellaneous/feedreader.h"

#include "services/standard/standardserviceentrypoint.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"
#include "services/abstract/serviceroot.h"

#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "core/feeddownloader.h"
#include "miscellaneous/databasecleaner.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"

#include <QThread>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>


FeedReader::FeedReader(QObject *parent)
  : QObject(parent), m_feedServices(QList<ServiceEntryPoint*>()),
    m_cacheSaveFutureWatcher(new QFutureWatcher<void>(this)), m_autoUpdateTimer(new QTimer(this)),
    m_feedDownloaderThread(nullptr), m_feedDownloader(nullptr),
    m_dbCleanerThread(nullptr), m_dbCleaner(nullptr) {
  m_feedsModel = new FeedsModel(this);
  m_feedsProxyModel = new FeedsProxyModel(m_feedsModel, this);
  m_messagesModel = new MessagesModel(this);
  m_messagesProxyModel = new MessagesProxyModel(m_messagesModel, this);

  connect(m_cacheSaveFutureWatcher, &QFutureWatcher<void>::finished, this, &FeedReader::asyncCacheSaveFinished);
  connect(m_autoUpdateTimer, &QTimer::timeout, this, &FeedReader::executeNextAutoUpdate);
  updateAutoUpdateStatus();
  asyncCacheSaveFinished();

  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool()) {
    qDebug("Requesting update for all feeds on application startup.");
    QTimer::singleShot(STARTUP_UPDATE_DELAY, this, SLOT(updateAllFeeds()));
  }
}

FeedReader::~FeedReader() {
  qDebug("Destroying FeedReader instance.");
  qDeleteAll(m_feedServices);
}

QList<ServiceEntryPoint*> FeedReader::feedServices() {
  if (m_feedServices.isEmpty()) {
    // NOTE: All installed services create their entry points here.
    m_feedServices.append(new StandardServiceEntryPoint());
    m_feedServices.append(new TtRssServiceEntryPoint());
    m_feedServices.append(new OwnCloudServiceEntryPoint());
  }

  return m_feedServices;
}

void FeedReader::updateFeeds(const QList<Feed*> &feeds) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    qApp->showGuiMessage(tr("Cannot update all items"),
                         tr("You cannot update all items because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);
    return;
  }

  if (m_feedDownloader == nullptr) {
    m_feedDownloader = new FeedDownloader();
    m_feedDownloaderThread = new QThread();

    // Downloader setup.
    qRegisterMetaType<QList<Feed*> >("QList<Feed*>");
    m_feedDownloader->moveToThread(m_feedDownloaderThread);

    connect(m_feedDownloaderThread, &QThread::finished, m_feedDownloaderThread, &QThread::deleteLater);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, this, &FeedReader::feedUpdatesFinished);
    connect(m_feedDownloader, &FeedDownloader::updateProgress, this, &FeedReader::feedUpdatesProgress);
    connect(m_feedDownloader, &FeedDownloader::updateStarted, this, &FeedReader::feedUpdatesStarted);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, qApp->feedUpdateLock(), &Mutex::unlock);

    // Connections are made, start the feed downloader thread.
    m_feedDownloaderThread->start();
  }

  QMetaObject::invokeMethod(m_feedDownloader, "updateFeeds", Q_ARG(QList<Feed*>, feeds));
}

void FeedReader::updateAutoUpdateStatus() {
  // Restore global intervals.
  // NOTE: Specific per-feed interval are left intact.
  m_globalAutoUpdateInitialInterval = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt();
  m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  m_globalAutoUpdateEnabled = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool();

  // Start global auto-update timer if it is not running yet.
  // NOTE: The timer must run even if global auto-update
  // is not enabled because user can still enable auto-update
  // for individual feeds.
  if (!m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->setInterval(AUTO_UPDATE_INTERVAL);
    m_autoUpdateTimer->start();
    qDebug("Auto-update timer started with interval %d.", m_autoUpdateTimer->interval());
  }
  else {
    qDebug("Auto-update timer is already running.");
  }
}

bool FeedReader::autoUpdateEnabled() const {
  return m_globalAutoUpdateEnabled;
}

int FeedReader::autoUpdateRemainingInterval() const {
  return m_globalAutoUpdateRemainingInterval;
}

int FeedReader::autoUpdateInitialInterval() const {
  return m_globalAutoUpdateInitialInterval;
}

void FeedReader::updateAllFeeds() {
  updateFeeds(m_feedsModel->rootItem()->getSubTreeFeeds());
}

void FeedReader::stopRunningFeedUpdate() {
  if (m_feedDownloader != nullptr) {
    QMetaObject::invokeMethod(m_feedDownloader, "stopRunningUpdate");
  }
}

bool FeedReader::isFeedUpdateRunning() const {
  return m_feedDownloader != nullptr && m_feedDownloader->isUpdateRunning();
}

DatabaseCleaner *FeedReader::databaseCleaner() {
  if (m_dbCleaner == nullptr) {
    m_dbCleaner = new DatabaseCleaner();
    m_dbCleanerThread = new QThread();

    // Downloader setup.
    qRegisterMetaType<CleanerOrders>("CleanerOrders");
    m_dbCleaner->moveToThread(m_dbCleanerThread);
    connect(m_dbCleanerThread, SIGNAL(finished()), m_dbCleanerThread, SLOT(deleteLater()));

    // Connections are made, start the feed downloader thread.
    m_dbCleanerThread->start();
  }

  return m_dbCleaner;
}

FeedDownloader *FeedReader::feedDownloader() const {
  return m_feedDownloader;
}

FeedsModel *FeedReader::feedsModel() const {
  return m_feedsModel;
}

MessagesModel *FeedReader::messagesModel() const {
  return m_messagesModel;
}

void FeedReader::executeNextAutoUpdate() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    qDebug("Delaying scheduled feed auto-updates for one minute due to another running update.");

    // Cannot update, quit.
    return;
  }

  // If global auto-update is enabled and its interval counter reached zero,
  // then we need to restore it.
  if (m_globalAutoUpdateEnabled && --m_globalAutoUpdateRemainingInterval < 0) {
    // We should start next auto-update interval.
    m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  }

  qDebug("Starting auto-update event, pass %d/%d.", m_globalAutoUpdateRemainingInterval, m_globalAutoUpdateInitialInterval);

  // Pass needed interval data and lets the model decide which feeds
  // should be updated in this pass.
  QList<Feed*> feeds_for_update = m_feedsModel->feedsForScheduledUpdate(m_globalAutoUpdateEnabled &&
                                                                        m_globalAutoUpdateRemainingInterval == 0);

  qApp->feedUpdateLock()->unlock();

  if (!feeds_for_update.isEmpty()) {
    // Request update for given feeds.
    updateFeeds(feeds_for_update);

    // NOTE: OSD/bubble informing about performing
    // of scheduled update can be shown now.
    if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::EnableAutoUpdateNotification)).toBool()) {
      qApp->showGuiMessage(tr("Starting auto-update of some feeds"),
                           tr("I will auto-update %n feed(s).", 0, feeds_for_update.size()),
                           QSystemTrayIcon::Information);
    }
  }
}

void FeedReader::checkServicesForAsyncOperations() {
  checkServicesForAsyncOperations(false);
}

void FeedReader::checkServicesForAsyncOperations(bool wait_for_future) {
  if (m_cacheSaveFutureWatcher->future().isRunning()) {
    qDebug("Previous future is still running.");

    // If we want to wait for future synchronously, we want to make sure that
    // we save all cached data (app exit).
    if (wait_for_future) {
      qWarning("Waiting for previously started saving of cached service data.");
      m_cacheSaveFutureWatcher->future().waitForFinished();
    }
    else {
      qWarning("Some cached service data are being saved now, so aborting this saving cycle.");
      // Some cache saving is now running.
      return;
    }
  }

  QFuture<void> future = QtConcurrent::run([&] {
    foreach (ServiceRoot *service, m_feedsModel->serviceRoots()) {
      // Store any cached data.
      service->saveAllCachedData();
    }
  });

  if (wait_for_future) {
    qDebug("Waiting for saving of cached service data to finish.");
    future.waitForFinished();
  }
  else {
    m_cacheSaveFutureWatcher->setFuture(future);
  }
}

void FeedReader::asyncCacheSaveFinished() {
  qDebug("I will start next check for cached service data in 30 seconds.");

  QTimer::singleShot(30000, [&] {
    qDebug("Starting next check for cached service data in NOW.");
    checkServicesForAsyncOperations(false);
  });
}

void FeedReader::quit() {
  if (m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->stop();
  }

  checkServicesForAsyncOperations(true);

  // Close worker threads.
  if (m_feedDownloaderThread != nullptr && m_feedDownloaderThread->isRunning()) {
    m_feedDownloader->stopRunningUpdate();

    if (m_feedDownloader->isUpdateRunning()) {
      QEventLoop loop(this);
      connect(m_feedDownloader, &FeedDownloader::updateFinished, &loop, &QEventLoop::quit);
      loop.exec();
    }

    qDebug("Quitting feed downloader thread.");
    m_feedDownloaderThread->quit();

    if (!m_feedDownloaderThread->wait(CLOSE_LOCK_TIMEOUT)) {
      qCritical("Feed downloader thread is running despite it was told to quit. Terminating it.");
      m_feedDownloaderThread->terminate();
    }
  }

  if (m_dbCleanerThread != nullptr && m_dbCleanerThread->isRunning()) {
    qDebug("Quitting database cleaner thread.");
    m_dbCleanerThread->quit();

    if (!m_dbCleanerThread->wait(CLOSE_LOCK_TIMEOUT)) {
      qCritical("Database cleaner thread is running despite it was told to quit. Terminating it.");
      m_dbCleanerThread->terminate();
    }
  }

  // Close workers.
  if (m_feedDownloader != nullptr) {
    qDebug("Feed downloader exists. Deleting it from memory.");
    m_feedDownloader->deleteLater();
  }

  if (m_dbCleaner != nullptr) {
    qDebug("Database cleaner exists. Deleting it from memory.");
    m_dbCleaner->deleteLater();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool()) {
    m_feedsModel->markItemCleared(m_feedsModel->rootItem(), true);
  }

  m_feedsModel->stopServiceAccounts();
}

MessagesProxyModel *FeedReader::messagesProxyModel() const {
  return m_messagesProxyModel;
}

FeedsProxyModel *FeedReader::feedsProxyModel() const {
  return m_feedsProxyModel;
}
