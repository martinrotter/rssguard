// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/feedreader.h"

#include "core/feeddownloader.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "gui/dialogs/formmessagefiltersmanager.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
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
    qDebugNN << LOGSEC_CORE
             << "Requesting update for all feeds on application startup.";
    QTimer::singleShot(qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateStartupDelay)).toDouble() * 1000,
                       this,
                       &FeedReader::updateAllFeeds);
  }
}

FeedReader::~FeedReader() {
  qDebugNN << LOGSEC_CORE << "Destroying FeedReader instance.";
  qDeleteAll(m_feedServices);
  qDeleteAll(m_messageFilters);
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
                         QSystemTrayIcon::MessageIcon::Warning, qApp->mainFormWidget(), true);
    return;
  }

  if (m_feedDownloader == nullptr) {
    qDebugNN << LOGSEC_CORE << "Creating FeedDownloader singleton.";

    m_feedDownloaderThread = new QThread();
    m_feedDownloader = new FeedDownloader();

    // Downloader setup.
    qRegisterMetaType<QList<Feed*>>("QList<Feed*>");

    m_feedDownloader->moveToThread(m_feedDownloaderThread);

    connect(m_feedDownloaderThread, &QThread::finished, m_feedDownloaderThread, &QThread::deleteLater);
    connect(m_feedDownloaderThread, &QThread::finished, m_feedDownloader, &FeedDownloader::deleteLater);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, this, &FeedReader::feedUpdatesFinished);
    connect(m_feedDownloader, &FeedDownloader::updateProgress, this, &FeedReader::feedUpdatesProgress);
    connect(m_feedDownloader, &FeedDownloader::updateStarted, this, &FeedReader::feedUpdatesStarted);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, qApp->feedUpdateLock(), &Mutex::unlock);

    m_feedDownloaderThread->start();
  }

  QMetaObject::invokeMethod(m_feedDownloader, "updateFeeds",
                            Qt::ConnectionType::QueuedConnection,
                            Q_ARG(QList<Feed*>, feeds));
}

void FeedReader::showMessageFiltersManager() {
  FormMessageFiltersManager manager(qApp->feedReader(),
                                    qApp->feedReader()->feedsModel()->serviceRoots(),
                                    qApp->mainFormWidget());

  manager.exec();
}

void FeedReader::updateAutoUpdateStatus() {
  // Restore global intervals.
  // NOTE: Specific per-feed interval are left intact.
  m_globalAutoUpdateInitialInterval = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt();
  m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  m_globalAutoUpdateEnabled = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool();
  m_globalAutoUpdateOnlyUnfocused = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateOnlyUnfocused)).toBool();

  // Start global auto-update timer if it is not running yet.
  // NOTE: The timer must run even if global auto-update
  // is not enabled because user can still enable auto-update
  // for individual feeds.
  if (!m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->setInterval(AUTO_UPDATE_INTERVAL);
    m_autoUpdateTimer->start();
    qDebugNN << LOGSEC_CORE << "Auto-update timer started with interval "
             << m_autoUpdateTimer->interval()
             << " ms.";
  }
  else {
    qDebugNN << LOGSEC_CORE << "Auto-update timer is already running.";
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

void FeedReader::loadSavedMessageFilters() {
  // Load all message filters from database.
  // All plugin services will hook active filters to
  // all feeds.
  m_messageFilters = DatabaseQueries::getMessageFilters(qApp->database()->connection(metaObject()->className()));

  for (auto* filter : m_messageFilters) {
    filter->setParent(this);
  }
}

MessageFilter* FeedReader::addMessageFilter(const QString& title, const QString& script) {
  auto* fltr = DatabaseQueries::addMessageFilter(qApp->database()->connection(metaObject()->className()), title, script);

  m_messageFilters.append(fltr);
  return fltr;
}

void FeedReader::removeMessageFilter(MessageFilter* filter) {
  m_messageFilters.removeAll(filter);

  // Now, remove all references from all feeds.
  auto all_feeds = m_feedsModel->feedsForIndex();

  for (auto* feed : all_feeds) {
    feed->removeMessageFilter(filter);
  }

  // Remove from DB.
  DatabaseQueries::removeMessageFilterAssignments(qApp->database()->connection(metaObject()->className()), filter->id());
  DatabaseQueries::removeMessageFilter(qApp->database()->connection(metaObject()->className()), filter->id());

  // Free from memory as last step.
  filter->deleteLater();
}

void FeedReader::updateMessageFilter(MessageFilter* filter) {
  DatabaseQueries::updateMessageFilter(qApp->database()->connection(metaObject()->className()), filter);
}

void FeedReader::assignMessageFilterToFeed(Feed* feed, MessageFilter* filter) {
  feed->appendMessageFilter(filter);
  DatabaseQueries::assignMessageFilterToFeed(qApp->database()->connection(metaObject()->className()),
                                             feed->customId(),
                                             filter->id(),
                                             feed->getParentServiceRoot()->accountId());
}

void FeedReader::removeMessageFilterToFeedAssignment(Feed* feed, MessageFilter* filter) {
  feed->removeMessageFilter(filter);
  DatabaseQueries::removeMessageFilterFromFeed(qApp->database()->connection(metaObject()->className()),
                                               feed->customId(),
                                               filter->id(),
                                               feed->getParentServiceRoot()->accountId());
}

void FeedReader::updateAllFeeds() {
  updateFeeds(m_feedsModel->rootItem()->getSubTreeFeeds());
}

void FeedReader::updateManuallyIntervaledFeeds() {
  updateFeeds(m_feedsModel->rootItem()->getSubTreeManuallyIntervaledFeeds());
}

void FeedReader::stopRunningFeedUpdate() {
  if (m_feedDownloader != nullptr) {
    m_feedDownloader->stopRunningUpdate();
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
  if (qApp->mainFormWidget()->isActiveWindow() && m_globalAutoUpdateOnlyUnfocused) {
    qDebugNN << LOGSEC_CORE
             << "Delaying scheduled feed auto-update for one minute since window is focused and updates while focused are disabled by the user.";

    // Cannot update, quit.
    return;
  }

  if (!qApp->feedUpdateLock()->tryLock()) {
    qDebugNN << LOGSEC_CORE << "Delaying scheduled feed auto-updates for one minute due to another running update.";

    // Cannot update, quit.
    return;
  }

  // If global auto-update is enabled and its interval counter reached zero,
  // then we need to restore it.
  if (m_globalAutoUpdateEnabled && --m_globalAutoUpdateRemainingInterval < 0) {
    // We should start next auto-update interval.
    m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  }

  qDebugNN << LOGSEC_CORE
           << "Starting auto-update event, pass "
           << m_globalAutoUpdateRemainingInterval << "/" << m_globalAutoUpdateInitialInterval << ".";

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
  for (ServiceRoot* service : m_feedsModel->serviceRoots()) {
    auto cache = dynamic_cast<CacheForServiceRoot*>(service);

    if (cache != nullptr) {
      cache->saveAllCachedData();
    }
  }

  asyncCacheSaveFinished();
}

void FeedReader::asyncCacheSaveFinished() {
  qDebugNN << LOGSEC_CORE << "I will start next check for cached service data in 60 seconds.";

  QTimer::singleShot(60000, this, [&] {
    qDebugNN << LOGSEC_CORE << "Saving cached metadata NOW.";
    checkServicesForAsyncOperations();
  });
}

QList<MessageFilter*> FeedReader::messageFilters() const {
  return m_messageFilters;
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

    // Both thread and downloader are auto-deleted when worker thread exits.
    m_feedDownloaderThread->quit();
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
