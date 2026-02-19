// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/feedreader.h"

#include "core/feeddownloader.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "database/databasequeries.h"
#include "gui/dialogs/formmessagefiltersmanager.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/pluginfactory.h"
#include "miscellaneous/settings.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceentrypoint.h"
#include "services/abstract/serviceroot.h"

#include <QThread>
#include <QTimer>

#if defined(ENABLE_TESTS)
#include <QAbstractItemModelTester>
#endif

#define ARTICLE_EXTRACTOR_NAME "rssguard-article-extractor"

FeedReader::FeedReader(QObject* parent)
  : QObject(parent), m_autoUpdateTimer(new QTimer(this)), m_feedDownloader(nullptr), m_feedFetchingPaused(false) {
  m_feedsModel = new FeedsModel(this);
  m_feedsProxyModel = new FeedsProxyModel(m_feedsModel, this);
  m_messagesModel = new MessagesModel(this);
  m_messagesProxyModel = new MessagesProxyModel(m_messagesModel, this);

#if defined(ENABLE_TESTS)
  new QAbstractItemModelTester(m_feedsModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
#endif

  updateAutoUpdateStatus();
  initializeFeedDownloader();

  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool()) {
    qDebugNN << LOGSEC_CORE << "Requesting update for all feeds on application startup.";
    QTimer::singleShot(qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateStartupDelay)).toDouble() * 1000,
                       this,
                       [this]() {
                         updateFeeds(m_feedsModel->rootItem()->getSubAutoFetchingEnabledFeeds());
                         connect(m_autoUpdateTimer, &QTimer::timeout, this, &FeedReader::executeNextAutoUpdate);
                       });
  }
  else {
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &FeedReader::executeNextAutoUpdate);
  }
}

FeedReader::~FeedReader() {
  qDebugNN << LOGSEC_CORE << "Destroying FeedReader instance.";

  for (ServiceEntryPoint* service : m_feedServices) {
    if (!service->isDynamicallyLoaded()) {
      qDebugNN << LOGSEC_CORE << "Deleting service" << QUOTE_W_SPACE_DOT(service->code());
      delete service;
    }
    else {
      qDebugNN << LOGSEC_CORE << "Service" << QUOTE_W_SPACE(service->code()) << "will be deleted by runtime.";
    }
  }
  // qDeleteAll(m_feedServices);

  qDeleteAll(m_messageFilters);
}

QList<ServiceEntryPoint*> FeedReader::feedServices() {
  if (m_feedServices.isEmpty()) {
    PluginFactory plugin_loader;

    // Add dynamically loaded plugins.
    m_feedServices.append(plugin_loader.loadPlugins());
  }

  return m_feedServices;
}

void FeedReader::updateFeeds(const QList<Feed*>& feeds, bool update_switched_off_too) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot fetch articles at this point"),
                          tr("You cannot fetch new articles now because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});
    return;
  }

  auto my_feeds = update_switched_off_too ? feeds
                                          : qlinq::from(feeds)
                                              .where([](const Feed* feed) {
                                                return !feed->isSwitchedOff();
                                              })
                                              .toList();

  if (my_feeds.isEmpty()) {
    qApp->feedUpdateLock()->unlock();
    return;
  }

  QMetaObject::invokeMethod(m_feedDownloader,
                            "updateFeeds",
                            Qt::ConnectionType::QueuedConnection,
                            Q_ARG(QList<Feed*>, my_feeds));
}

void FeedReader::synchronizeMessageData(const QList<CacheForServiceRoot*>& caches) {
  QMetaObject::invokeMethod(m_feedDownloader,
                            "synchronizeAccountCaches",
                            Qt::ConnectionType::QueuedConnection,
                            Q_ARG(QList<CacheForServiceRoot*>, caches),
                            Q_ARG(bool, true));
}

void FeedReader::initializeFeedDownloader() {
  if (m_feedDownloader == nullptr) {
    qDebugNN << LOGSEC_CORE << "Creating FeedDownloader singleton.";

    m_feedDownloader = new FeedDownloader();
    m_feedDownloaderThread = new QThread();

    // Downloader setup.
    qRegisterMetaType<QList<Feed*>>("QList<Feed*>");
    qRegisterMetaType<QList<CacheForServiceRoot*>>("QList<CacheForServiceRoot*>");

    m_feedDownloader->moveToThread(m_feedDownloaderThread);

    connect(m_feedDownloaderThread, &QThread::finished, m_feedDownloaderThread, &QThread::deleteLater);
    connect(m_feedDownloaderThread, &QThread::finished, m_feedDownloader, &FeedDownloader::deleteLater);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, this, &FeedReader::onFeedUpdatesFinished);
    connect(m_feedDownloader, &FeedDownloader::updateProgress, this, &FeedReader::feedUpdatesProgress);
    connect(m_feedDownloader, &FeedDownloader::updateStarted, this, &FeedReader::feedUpdatesStarted);
    connect(m_feedDownloader, &FeedDownloader::updateFinished, qApp->feedUpdateLock(), &Mutex::unlock);

    m_feedDownloaderThread->start(QThread::Priority::LowPriority);
  }
}

QDateTime FeedReader::lastAutoUpdate() const {
  return m_lastAutoUpdate;
}

QByteArray FeedReader::exportMessageFilters() const {
  QJsonArray arr;
  auto flt = messageFilters();
  auto flt_ordered = qlinq::from(flt).orderBy([](MessageFilter* f) {
    return f->sortOrder();
  });

  for (auto* fltr : std::as_const(flt_ordered)) {
    QJsonObject obj;

    obj.insert(QSL("name"), fltr->name());
    obj.insert(QSL("script"), fltr->script());

    arr.append(obj);
  }

  return QJsonDocument(arr).toJson(QJsonDocument::JsonFormat::Indented);
}

void FeedReader::importMessageFilters(const QByteArray& data) {
  QJsonParseError err;
  const QJsonArray arr = QJsonDocument::fromJson(data, &err).array();

  for (const QJsonValue& json_fltr_val : arr) {
    const QJsonObject& json_fltr = json_fltr_val.toObject();
    const QString name = json_fltr.value(QSL("name")).toString();
    const QString script = json_fltr.value(QSL("script")).toString();

    addMessageFilter(name, script);
  }
}

void FeedReader::showMessageFiltersManager() {
  FormMessageFiltersManager manager(qApp->feedReader(),
                                    qApp->feedReader()->feedsModel()->serviceRoots(),
                                    qApp->mainFormWidget());

  manager.exec();

  m_feedsModel->reloadCountsOfWholeModel();
  m_messagesModel->reloadWholeLayout();
}

QString FeedReader::getFullArticle(const QUrl& url, bool plain_text_only) const {
  static QString article_extractor = qApp->applicationDirPath() + QDir::separator() + QSL(ARTICLE_EXTRACTOR_NAME);

  QStringList params = {url.toString()};

  if (plain_text_only) {
    params.prepend(QSL("-t"));
  }

  const QString output = IOFactory::startProcessGetOutput(article_extractor, params);

  return output;
}

void FeedReader::updateAutoUpdateStatus() {
  // Restore global intervals.
  // NOTE: Specific per-feed interval are left intact.
  m_globalAutoUpdateInterval = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt();
  m_globalAutoUpdateFast = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FastAutoUpdate)).toBool();

  if (m_lastAutoUpdate.isNull()) {
    m_lastAutoUpdate = QDateTime::currentDateTimeUtc();
  }

  m_globalAutoUpdateEnabled = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool();
  m_globalAutoUpdateOnlyUnfocused =
    qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateOnlyUnfocused)).toBool();

  if (m_globalAutoUpdateFast) {
    // NOTE: In "fast" mode, we set interval to 1 second.
    // This might have some performance consequences.
    m_autoUpdateTimer->setInterval(1000);
    qDebugNN << LOGSEC_CORE
             << "Enabling support for very small auto-fetching intervals. This might have performance consequences.";
  }
  else {
    m_autoUpdateTimer->setInterval(AUTO_UPDATE_INTERVAL * 1000);
  }

  // Start global auto-update timer if it is not running yet.
  // NOTE: The timer must run even if global auto-update
  // is not enabled because user can still enable auto-update
  // for individual feeds.
  if (!m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->start();
    qDebugNN << LOGSEC_CORE << "Auto-download timer started with interval " << m_autoUpdateTimer->interval() << " ms.";
  }
  else {
    qDebugNN << LOGSEC_CORE << "Auto-download timer is already running.";
  }
}

bool FeedReader::autoUpdateEnabled() const {
  return m_globalAutoUpdateEnabled;
}

int FeedReader::autoUpdateInterval() const {
  return m_globalAutoUpdateInterval;
}

void FeedReader::loadSavedMessageFilters() {
  // Load all message filters from database.
  // All plugin services will hook active filters to
  // all feeds.
  m_messageFilters = qApp->database()->worker()->read<QList<MessageFilter*>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getMessageFilters(db);
  });

  for (auto* filter : std::as_const(m_messageFilters)) {
    filter->setParent(this);
  }
}

MessageFilter* FeedReader::addMessageFilter(const QString& title, const QString& script) {
  auto fltr_names = qlinq::from(m_messageFilters)
                      .select([](const MessageFilter* fl) {
                        return fl->name();
                      })
                      .toList();

  auto unique_title = TextFactory::ensureUniqueName(title, fltr_names);

  auto* fltr = qApp->database()->worker()->write<MessageFilter*>([&](const QSqlDatabase& db) {
    return DatabaseQueries::addMessageFilter(db, unique_title, script);
  });

  m_messageFilters.append(fltr);
  return fltr;
}

void FeedReader::removeMessageFilter(MessageFilter* filter) {
  // Now, remove all references from all feeds.
  auto all_feeds = m_feedsModel->feedsForIndex();

  for (auto* feed : all_feeds) {
    feed->removeMessageFilter(filter);
  }

  // Remove from DB.
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::moveMessageFilter(m_messageFilters, filter, false, true, {}, db);
    DatabaseQueries::removeMessageFilter(db, filter->id());
  });

  m_messageFilters.removeAll(filter);

  // Free from memory as last step.
  filter->deleteLater();
}

void FeedReader::updateMessageFilter(MessageFilter* filter) {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::updateMessageFilter(db, filter);
  });
}

void FeedReader::assignMessageFilterToFeed(Feed* feed, MessageFilter* filter) {
  feed->appendMessageFilter(filter);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::assignMessageFilterToFeed(db, feed->id(), filter->id());
  });
}

void FeedReader::removeMessageFilterToFeedAssignment(Feed* feed, MessageFilter* filter) {
  feed->removeMessageFilter(filter);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::removeMessageFilterFromFeed(db, feed->id(), filter->id());
  });
}

void FeedReader::updateAllFeeds() {
  updateFeeds(m_feedsModel->rootItem()->getSubTreeFeeds());
}

void FeedReader::updateManuallyIntervaledFeeds() {
  updateFeeds(m_feedsModel->rootItem()->getSubTreeAutoFetchingWithManualIntervalsFeeds());
}

void FeedReader::stopRunningFeedUpdate() {
  if (m_feedDownloader != nullptr) {
    m_feedDownloader->stopRunningUpdate();
  }
}

void FeedReader::pauseUnpaseFeedFetching(bool pause) {
  m_feedFetchingPaused = pause;
  qApp->settings()->setValue(GROUP(Feeds), Feeds::PauseFeedFetching, pause);
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
  bool disable_update_with_window =
    (qApp->mainFormWidget()->isActiveWindow() || QApplication::activeModalWidget() != nullptr) &&
    m_globalAutoUpdateOnlyUnfocused;
  auto roots = qApp->feedReader()->feedsModel()->serviceRoots();
  auto full_caches = qlinq::from(roots)
                       .select([](ServiceRoot* root) -> CacheForServiceRoot* {
                         auto* cache = root->toCache();

                         if (cache != nullptr) {
                           return cache;
                         }
                         else {
                           return nullptr;
                         }
                       })
                       .where([](CacheForServiceRoot* cache) {
                         return cache != nullptr && !cache->isEmpty();
                       });

  // Skip this round of auto-updating, but only if user disabled it when main window is active
  // and there are no caches to synchronize.
  if ((m_feedFetchingPaused || disable_update_with_window) && full_caches.isEmpty()) {
    qDebugNN << LOGSEC_CORE << "Delaying scheduled feed auto-download for some time since window "
             << "is focused and updates while focused are disabled by the "
             << "user (or paused) and all account caches are empty.";

    // Cannot update, quit.
    return;
  }

  if (!qApp->feedUpdateLock()->tryLock()) {
    qDebugNN << LOGSEC_CORE << "Delaying scheduled feed auto-downloads and message state synchronization for "
             << "some time due to another running update.";
    // Cannot update, quit.
    return;
  }

  qApp->feedUpdateLock()->unlock();

  // Resynchronize caches.
  if (!full_caches.isEmpty()) {
    QList<CacheForServiceRoot*> caches = full_caches.toList();
    synchronizeMessageData(caches);
  }

  if (m_feedFetchingPaused || disable_update_with_window) {
    qDebugNN << LOGSEC_CORE << "Delaying scheduled feed auto-download for some time since window "
             << "is focused or feed fetching is paused. Article cache was synchronised nonetheless.";
    return;
  }

  // Pass needed interval data and lets the model decide which feeds
  // should be updated in this pass.
  QDateTime current_time = QDateTime::currentDateTimeUtc();
  bool auto_update_now =
    m_globalAutoUpdateEnabled && m_lastAutoUpdate.addSecs(m_globalAutoUpdateInterval) < current_time;

  if (auto_update_now) {
    qDebugNN << LOGSEC_CORE << "Now it's time to auto-fetch articles because last auto-fetch was on"
             << QUOTE_W_SPACE(m_lastAutoUpdate) << "and next should be in"
             << NONQUOTE_W_SPACE(m_globalAutoUpdateInterval) << "seconds.";

    m_lastAutoUpdate = current_time;
  }

  QList<Feed*> feeds_for_update = m_feedsModel->feedsForScheduledUpdate(auto_update_now);

  if (!feeds_for_update.isEmpty()) {
    // Request update for given feeds.
    updateFeeds(feeds_for_update);

    if (qlinq::from(feeds_for_update).any([](const Feed* fd) {
          return !fd->isQuiet();
        })) {
      // NOTE: OSD/bubble informing about performing of scheduled update can be shown now.
      qApp->showGuiMessage(Notification::Event::ArticlesFetchingStarted,
                           {tr("Starting auto-download of some feeds' articles"),
                            tr("I will auto-download new articles for %n feed(s).", nullptr, feeds_for_update.size()),
                            QSystemTrayIcon::MessageIcon::Information});
    }
  }
}

void FeedReader::onFeedUpdatesFinished(FeedDownloadResults updated_feeds) {
  // NOTE: Keep in sync with the same line in FeedDownloader::updateOneFeed() method.
  const bool update_feed_list =
    qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateFeedListDuringFetching)).toBool() &&
    updated_feeds.feedRequests().size() <= 25;

  if (!update_feed_list) {
    // NOTE: Counts were not updated during feed fetching, update them now.
    for (auto* acc : updated_feeds.updatedAccounts()) {
      acc->updateCounts();
    }
  }

  m_feedsModel->reloadWholeLayout();
  m_feedsModel->notifyWithCounts();

  if (!updated_feeds.erroredFeeds().isEmpty()) {
    qApp->showGuiMessage(Notification::Event::ArticlesFetchingError,
                         {tr("Some feeds have errors"),
                          tr("Some feeds threw an error when fetching articles."),
                          QSystemTrayIcon::MessageIcon::Warning});
  }

  emit feedUpdatesFinished(updated_feeds);
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

    if (m_feedDownloader->isUpdateRunning() || m_feedDownloader->isCacheSynchronizationRunning()) {
      QEventLoop loop(this);

      connect(m_feedDownloader, &FeedDownloader::cachesSynchronized, &loop, &QEventLoop::quit);
      connect(m_feedDownloader, &FeedDownloader::updateFinished, &loop, &QEventLoop::quit);
      loop.exec();
    }

    // Both thread and downloader are auto-deleted when worker thread exits.
    m_feedDownloaderThread->quit();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool()) {
    try {
      m_feedsModel->markItemCleared(m_feedsModel->rootItem(), true);
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_CORE << "Cannot clear items:" << NONQUOTE_W_SPACE_DOT(ex.message());
    }
  }

  m_feedsModel->stopServiceAccounts();
}

MessagesProxyModel* FeedReader::messagesProxyModel() const {
  return m_messagesProxyModel;
}

FeedsProxyModel* FeedReader::feedsProxyModel() const {
  return m_feedsProxyModel;
}
