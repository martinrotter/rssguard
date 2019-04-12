// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/feedreader.h"

#include "core/feeddownloader.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"
#include "services/gmail/gmailentrypoint.h"
#include "services/inoreader/inoreaderentrypoint.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QThread>
#include <QTimer>

FeedReader::FeedReader(QObject* parent)
  : QObject(parent),
  m_autoUpdateTimer(new QTimer(this)), m_feedDownloader(nullptr) {
  m_feedsModel = new FeedsModel(this);
  m_feedsProxyModel = new FeedsProxyModel(m_feedsModel, this);
  m_messagesModel = new MessagesModel(this);
  m_messagesProxyModel = new MessagesProxyModel(m_messagesModel, this);

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
    m_feedServices.append(new GmailEntryPoint());
    m_feedServices.append(new InoreaderEntryPoint());
    m_feedServices.append(new OwnCloudServiceEntryPoint());
    m_feedServices.append(new StandardServiceEntryPoint());
    m_feedServices.append(new TtRssServiceEntryPoint());
  }

  return m_feedServices;
}

void FeedReader::updateFeeds(const QList<Feed*>& feeds) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    qApp->showGuiMessage(tr("Cannot update all items"),
                         tr("You cannot update all items because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);
    return;
  }

  if (m_feedDownloader == nullptr) {
    qDebug("Creating FeedDownloader singleton.");

    m_feedDownloader = new FeedDownloader();

    // Downloader setup.
    qRegisterMetaType<QList<Feed*>>("QList<Feed*>");

    connect(m_feedDownloader, &FeedDownloader::updateFinished, this, &FeedReader::feedUpdatesFinished);
    connect(m_feedDownloader, &FeedDownloader::updateProgress, this, &FeedReader::feedUpdatesProgress);
    connect(m_feedDownloader, &FeedDownloader::updateStarted, this, &FeedReader::feedUpdatesStarted);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, qApp->feedUpdateLock(), &Mutex::unlock);
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
    m_feedDownloader->stopRunningUpdate();

    //QMetaObject::invokeMethod(m_feedDownloader, "stopRunningUpdate");
  }
}

bool FeedReader::isFeedUpdateRunning() const {
  return m_feedDownloader != nullptr && m_feedDownloader->isUpdateRunning();
}

FeedDownloader* FeedReader::feedDownloader() const {
  return m_feedDownloader;
}

FeedsModel* FeedReader::feedsModel() const {
  return m_feedsModel;
}

MessagesModel* FeedReader::messagesModel() const {
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
                           tr("I will auto-update %n feed(s).", nullptr, feeds_for_update.size()),
                           QSystemTrayIcon::Information);
    }
  }
}

void FeedReader::checkServicesForAsyncOperations() {
  foreach (ServiceRoot* service, m_feedsModel->serviceRoots()) {
    auto cache = dynamic_cast<CacheForServiceRoot*>(service);

    if (cache != nullptr) {
      cache->saveAllCachedData();
    }
  }

  asyncCacheSaveFinished();
}

void FeedReader::asyncCacheSaveFinished() {
  qDebug("I will start next check for cached service data in 30 seconds.");
  QTimer::singleShot(60000, [&] {
    qDebug("Starting next check for cached service data in NOW.");
    checkServicesForAsyncOperations();
  });
}

void FeedReader::quit() {
  if (m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->stop();
  }

  // Stop running updates.
  if (m_feedDownloader != nullptr) {
    m_feedDownloader->stopRunningUpdate();

    if (m_feedDownloader->isUpdateRunning()) {
      QEventLoop loop(this);

      connect(m_feedDownloader, &FeedDownloader::updateFinished, &loop, &QEventLoop::quit);
      loop.exec();
    }
  }

  // Close workers.
  if (m_feedDownloader != nullptr) {
    qDebug("Feed downloader exists. Deleting it from memory.");
    m_feedDownloader->deleteLater();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool()) {
    m_feedsModel->markItemCleared(m_feedsModel->rootItem(), true);
  }

  m_feedsModel->stopServiceAccounts();
}

MessagesProxyModel* FeedReader::messagesProxyModel() const {
  return m_messagesProxyModel;
}

FeedsProxyModel* FeedReader::feedsProxyModel() const {
  return m_feedsProxyModel;
}
