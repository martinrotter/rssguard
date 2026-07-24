// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"

#include "core/feedsmodel.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/notifications/toastnotificationsmanager.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "gui/toolbars/statusbar.h"
#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"
#include "miscellaneous/applicationlifecycle.h"
#include "miscellaneous/applicationlogmanager.h"
#include "miscellaneous/applicationpaths.h"
#include "miscellaneous/commandlinecontroller.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/guinotificationcoordinator.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/notificationfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/thread.h"
#include "miscellaneous/windowstaskbar.h"
#include "network-web/webfactory.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/serviceroot.h"

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include "gui/webviewers/qtwebengine/webengineviewer.h"
#endif

#include <QAction>
#include <QEventLoop>
#include <QIcon>
#include <QLoggingCategory>
#include <QPixmap>
#include <QProcess>
#include <QSplashScreen>
#include <QSslSocket>
#include <QThreadPool>
#include <QTimer>
#include <QVersionNumber>

#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
#include <QQuickWindow>
#endif

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include <QWebEngineUrlScheme>
#endif

Application::Application(const QString& id, int& argc, char** argv, const QStringList& raw_cli_args)
  : SingleApplication(id, argc, argv), m_updateFeedsLock(new Mutex()) {
#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
  // HACK: Force rendering system to use OpenGL backend.
#if QT_VERSION_MAJOR < 6
  QQuickWindow::setSceneGraphBackend(QSGRendererInterface::GraphicsApi::OpenGL);
#else
  QQuickWindow::setGraphicsApi(QSGRendererInterface::GraphicsApi::OpenGL);
#endif
#endif

  QString custom_ua;

  m_paths.reset(new ApplicationPaths(this));
  m_commandLine.reset(new CommandLineController(this, m_paths.data(), raw_cli_args));
  m_logManager.reset(new ApplicationLogManager(this));

  parseCmdArgumentsFromMyInstance(raw_cli_args, custom_ua);
  qInstallMessageHandler(performLogging);

  m_feedReader = nullptr;
  m_lifecycle.reset(new ApplicationLifecycle(this));
  m_mainForm = nullptr;

  m_settings = nullptr;
  m_webFactory = nullptr;
  m_system = nullptr;
  m_skins = nullptr;
  m_localization = nullptr;
  m_icons = nullptr;
  m_database = nullptr;
  m_notifications = nullptr;
  m_toastNotifications = nullptr;
#if QT_VERSION_MAJOR > 5
  m_workHorsePool = nullptr;
#endif
  m_settings = Settings::setupSettings(this);

  initializeSplash();
  showSplashMessage(tr("Initializing application..."));

  m_logManager->initializeFileBasedLogging();

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  if (!cmdParser()->isSet(QSL(CLI_FORCETEXT_LONG))) {
    QString existing_flags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS");
    QString flags = settings()->value(GROUP(Web), SETTING(Web::WebEngineChromiumFlags)).toString().trimmed();
    QString final_flags = WebFactory::injectPacIntoChromiumFlags(existing_flags, flags);

    if (!final_flags.isEmpty()) {
      qDebugNN << LOGSEC_CORE << "Setting QTWEBENGINE_CHROMIUM_FLAGS to" << QUOTE_W_SPACE_DOT(final_flags);
      qputenv("QTWEBENGINE_CHROMIUM_FLAGS", final_flags.toLocal8Bit());
    }
  }
#endif

  m_localization = new Localization(this);

  m_localization->loadActiveLanguage();
  showSplashMessage(tr("Initializing application services..."));

#if QT_VERSION_MAJOR > 5
  m_workHorsePool = new QThreadPool(this);
#endif

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  QWebEngineUrlScheme gemini_scheme("gemini");
  gemini_scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);

  QWebEngineUrlScheme::registerScheme(gemini_scheme);
#endif

  m_webFactory = new WebFactory(this);
  m_system = new SystemFactory(this);
  m_skins = new SkinFactory(this);
  m_icons = new IconFactory(this);
  m_database = new DatabaseFactory(this);
  m_notifications = new NotificationFactory(this);
  m_toastNotifications =
    (!isWayland() && m_notifications->useToastNotifications()) ? new ToastNotificationsManager(this) : nullptr;
  m_guiNotifications.reset(new GuiNotificationCoordinator(this));

#if defined(Q_OS_WIN)
  m_windowsTaskbar.reset(new WindowsTaskbar(this));
#endif

  determineFirstRuns();

  // Add an extra path for non-system icon themes and set current icon theme
  // and skin.
  showSplashMessage(tr("Loading appearance..."));
  m_icons->setupSearchPaths();
  m_icons->loadCurrentIconTheme();

  reloadCurrentSkin(false);
  setupFont();
  showSplashMessage(tr("Preparing the application..."));

  if (m_toastNotifications != nullptr) {
    connect(m_toastNotifications,
            &ToastNotificationsManager::openingArticleInArticleListRequested,
            this,
            &Application::loadMessageToFeedAndArticleList);
  }

  connect(this, &Application::aboutToQuit, m_lifecycle.data(), &ApplicationLifecycle::onAboutToQuit);
  connect(this, &Application::commitDataRequest, m_lifecycle.data(), &ApplicationLifecycle::onCommitData);
  connect(this, &Application::saveStateRequest, m_lifecycle.data(), &ApplicationLifecycle::onSaveState);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  QString app_dir = QString::fromLocal8Bit(qgetenv("APPDIR"));

  if (!app_dir.isEmpty()) {
    bool success = qputenv("GST_PLUGIN_SYSTEM_PATH_1_0",
                           QSL("%1/usr/lib/gstreamer-1.0:%2")
                             .arg(app_dir, QString::fromLocal8Bit(qgetenv("GST_PLUGIN_SYSTEM_PATH_1_0")))
                             .toLocal8Bit());
    success = qputenv("GST_PLUGIN_SCANNER_1_0",
                      QSL("%1/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner").arg(app_dir).toLocal8Bit()) &&
              success;
    if (!success) {
      qWarningNN << LOGSEC_CORE << "Unable to set up GStreamer environment.";
    }
  }
#endif

  if (!custom_ua.isEmpty()) {
    m_webFactory->setCustomUserAgent(custom_ua);
  }
  else {
    custom_ua = qApp->settings()->value(GROUP(Network), SETTING(Network::CustomUserAgent)).toString();
    m_webFactory->setCustomUserAgent(custom_ua);
  }

  m_webFactory->updateProxy();

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  m_webFactory->updateWebEngineProfileSettings();
#endif

  if (isFirstRun()) {
    m_notifications->save({Notification(Notification::Event::GeneralEvent, true),
                           Notification(Notification::Event::NewUnreadArticlesFetched,
                                        true,
                                        false,
                                        true,
                                        QSL("%1/notify.wav").arg(SOUNDS_BUILTIN_DIRECTORY)),
                           Notification(Notification::Event::NewAppVersionAvailable, true),
                           Notification(Notification::Event::LoginProgressOrSuccessful, true),
                           Notification(Notification::Event::LoginFailure, true)},
                          settings());
  }
  else {
    m_notifications->load(settings());
  }

  QTimer::singleShot(30000, system(), &SystemFactory::checkForUpdatesOnStartup);

  setupWorkHorsePool();
  updateCliDebugStatus();

#if QT_VERSION >= 0x060100 // Qt >= 6.1.0
  QSslSocket::setActiveBackend(QSL("openssl"));
#endif

  const QString app_revision = QSL(APP_REVISION).isEmpty() ? QSL("unknown") : QSL(APP_REVISION);

  qDebugNN << LOGSEC_CORE << "RSS Guard version:" << QUOTE_W_SPACE(QSL(APP_VERSION))
           << "revision:" << QUOTE_W_SPACE_DOT(app_revision);
  qDebugNN << LOGSEC_CORE << "Platform:" << QUOTE_W_SPACE_DOT(QGuiApplication::platformName());
  qDebugNN << LOGSEC_CORE << "DB version:" << QUOTE_W_SPACE_DOT(qApp->database()->driver()->version());

#if QT_VERSION >= 0x060100 // Qt >= 6.1.0
  qDebugNN << LOGSEC_CORE << "OpenSSL backends:" << QUOTE_W_SPACE_DOT(QSslSocket::availableBackends());
#endif

  qDebugNN << LOGSEC_CORE << "OpenSSL version:" << QUOTE_W_SPACE_DOT(QSslSocket::sslLibraryVersionString());
  qDebugNN << LOGSEC_CORE << "OpenSSL supported:" << QUOTE_W_SPACE_DOT(QSslSocket::supportsSsl());
  qDebugNN << LOGSEC_CORE << "Global thread pool has"
           << NONQUOTE_W_SPACE(QThreadPool::globalInstance()->maxThreadCount()) << "threads.";
  qDebugNN << LOGSEC_CORE << "Main thread ID:" << QUOTE_W_SPACE_DOT(getThreadID());
  qDebugNN << LOGSEC_CORE
           << "Uses system proxy config:" << QUOTE_W_SPACE_DOT(QNetworkProxyFactory::usesSystemConfiguration());
}

Application::~Application() {
  m_logManager->shutdown();

  qDebugNN << LOGSEC_CORE << "Destroying Application instance.";
}

void Application::initializeSplash() {
  if (!settings()->value(GROUP(General), SETTING(General::ShowSplashScreen)).toBool()) {
    return;
  }

  QPixmap banner(QSL(":/graphics/banner.png"));

  if (banner.isNull()) {
    return;
  }

  banner = banner.scaledToWidth(480, Qt::TransformationMode::SmoothTransformation);

  m_splashScreen.reset(new QSplashScreen(banner));
  m_splashScreen->show();

  processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
}

void Application::showSplashMessage(const QString& message) {
  if (m_splashScreen == nullptr) {
    return;
  }

  m_splashScreen->showMessage(message,
                              Qt::AlignmentFlag::AlignBottom | Qt::AlignmentFlag::AlignHCenter,
                              QColor(QSL("#3A2520")));
  processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
}

void Application::finishSplash(QWidget* main_window) {
  if (m_splashScreen == nullptr) {
    return;
  }

  if (main_window != nullptr && main_window->isVisible()) {
    m_splashScreen->finish(main_window);
  }
  else {
    m_splashScreen->close();
  }

  m_splashScreen.reset();
}

void Application::updateCliDebugStatus() {
  m_logManager->updateCliDebugStatus();
}

void Application::performLogging(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
  ApplicationLogManager::performLogging(type, context, msg);
}

void Application::reactOnForeignNotifications() {
  connect(this, &Application::messageReceived, this, &Application::parseCmdArgumentsFromOtherInstance);
}

void Application::hideOrShowMainForm() {
  // Display main window.
  if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool()) {
    qDebugNN << LOGSEC_CORE << "Hiding the main window when the application is starting.";
    mainForm()->switchVisibility(true);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Showing the main window when the application is starting.";
    mainForm()->show();
  }

  mainForm()->tabWidget()->feedMessageViewer()->feedsView()->setFocus();
}

void Application::loadDynamicShortcuts() {
  DynamicShortcuts::load(userAndExtraActions());
}

void Application::offerPolls() const {
  m_guiNotifications->offerPolls();
}

void Application::offerChanges() {
  m_guiNotifications->offerChanges();
}

bool Application::isAlreadyRunning() {
  return m_commandLine->isAlreadyRunning();
}

QStringList Application::builtinSounds() const {
  auto builtin_sounds = QDir(QSL(SOUNDS_BUILTIN_DIRECTORY)).entryInfoList(QDir::Filter::Files, QDir::SortFlag::Name);
  auto builtin_sounds_paths = qlinq::from(builtin_sounds).select([](const QFileInfo& i) {
    return i.absoluteFilePath();
  });

  return builtin_sounds_paths.toList();
}

FeedReader* Application::feedReader() {
  return m_feedReader;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != nullptr && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();
  }

  return m_userActions;
}

QList<QAction*> Application::userAndExtraActions() {
  auto user_actions = userActions();

  user_actions << m_mainForm->tabWidget()->feedMessageViewer()->feedsToolBar()->extraActions();
  user_actions << m_mainForm->tabWidget()->feedMessageViewer()->messagesToolBar()->extraActions();
  user_actions << m_mainForm->statusBar()->extraActions();

  return user_actions;
}

bool Application::isFirstRun() const {
  return m_firstRunEver;
}

bool Application::isFirstRunCurrentVersion() const {
  return m_firstRunCurrentVersion;
}

QCommandLineParser* Application::cmdParser() {
  return m_commandLine->parser();
}

WebFactory* Application::web() const {
  return m_webFactory;
}

SystemFactory* Application::system() {
  return m_system;
}

SkinFactory* Application::skins() {
  return m_skins;
}

Localization* Application::localization() {
  return m_localization;
}

DatabaseFactory* Application::database() {
  return m_database;
}

void Application::eliminateFirstRuns() {
  settings()->setValue(GROUP(General), General::FirstRun, false);
  settings()->setValue(GROUP(General), QString(General::FirstRun) + QL1C('_') + APP_VERSION, false);
}

ToastNotificationsManager* Application::toastNotifications() const {
  return m_toastNotifications;
}

#if QT_VERSION_MAJOR > 5
QThreadPool* Application::workHorsePool() const {
  return m_workHorsePool;
}
#endif

QStringList Application::rawCliArgs() const {
  return m_commandLine->rawArguments();
}

NotificationFactory* Application::notifications() const {
  return m_notifications;
}

void Application::setFeedReader(FeedReader* feed_reader) {
  m_feedReader = feed_reader;

  connect(m_feedReader,
          &FeedReader::feedUpdatesStarted,
          m_guiNotifications.data(),
          &GuiNotificationCoordinator::onFeedUpdatesStarted);
  connect(m_feedReader,
          &FeedReader::feedUpdatesProgress,
          m_guiNotifications.data(),
          &GuiNotificationCoordinator::onFeedUpdatesProgress);
  connect(m_feedReader,
          &FeedReader::feedUpdatesFinished,
          m_guiNotifications.data(),
          &GuiNotificationCoordinator::onFeedUpdatesFinished);
  connect(m_feedReader->feedsModel(),
          &FeedsModel::messageCountsChanged,
          m_guiNotifications.data(),
          &GuiNotificationCoordinator::showMessagesNumber);
}

IconFactory* Application::icons() {
  return m_icons;
}

Settings* Application::settings() const {
  return m_settings;
}

Mutex* Application::feedUpdateLock() {
  return m_updateFeedsLock.data();
}

FormMain* Application::mainForm() {
  return m_mainForm;
}

QWidget* Application::mainFormWidget() {
  return m_mainForm;
}

#if defined(Q_OS_WIN)
WindowsTaskbar* Application::windowsTaskbar() const {
  return m_windowsTaskbar.data();
}
#endif

void Application::setMainForm(FormMain* main_form) {
  m_mainForm = main_form;

#if defined(Q_OS_WIN)
  if (m_windowsTaskbar != nullptr) {
    m_windowsTaskbar->setThumbnailActions(m_mainForm->taskbarThumbnailActions(),
                                          icons()->fromTheme(QSL("media-playback-pause"), QSL("player_pause")),
                                          icons()->fromTheme(QSL("media-playback-start"), QSL("player_play")));
    m_windowsTaskbar
      ->setThumbnailButtonsEnabled(settings()->value(GROUP(GUI), SETTING(GUI::TaskbarThumbnailButtons)).toBool());
  }
#endif

  m_guiNotifications->setMainForm();
}

QString Application::configFolder() const {
  return m_paths->configFolder();
}

QString Application::userDataAppFolder() const {
  return m_paths->userDataAppFolder();
}

QString Application::userDataFolder() {
  return m_paths->userDataFolder();
}

QString Application::replaceUserDataFolderPlaceholder(QString text, bool double_escape) const {
  return m_paths->replaceUserDataFolderPlaceholder(std::move(text), double_escape);
}

QStringList Application::replaceUserDataFolderPlaceholder(QStringList texts) const {
  return m_paths->replaceUserDataFolderPlaceholder(std::move(texts));
}

QString Application::userDataHomeFolder() const {
  return m_paths->userDataHomeFolder();
}

QString Application::tempFolder() const {
  return m_paths->tempFolder();
}

QString Application::documentsFolder() const {
  return m_paths->documentsFolder();
}

QString Application::homeFolder() const {
  return m_paths->homeFolder();
}

void Application::backupDatabaseSettings(bool backup_database,
                                         bool backup_settings,
                                         const QString& target_path,
                                         const QString& backup_name) {
  if (!QFileInfo(target_path).isWritable()) {
    throw ApplicationException(tr("Output directory is not writable."));
  }

  if (backup_settings) {
    settings()->sync();

    if (!IOFactory::copyFile(settings()->fileName(),
                             target_path + QDir::separator() + backup_name + BACKUP_SUFFIX_SETTINGS)) {
      throw ApplicationException(tr("Settings file not copied to output directory successfully."));
    }
  }

  if (backup_database) {
    database()->driver()->backupDatabase(target_path, backup_name);
  }
}

void Application::restoreDatabaseSettings(bool restore_database,
                                          bool restore_settings,
                                          const QString& source_database_file_path,
                                          const QString& source_settings_file_path) {
  if (restore_database) {
    qApp->database()->driver()->initiateRestoration(source_database_file_path);
  }

  if (restore_settings) {
    if (!qApp->settings()->initiateRestoration(source_settings_file_path)) {
      throw ApplicationException(tr("Settings restoration was not initiated. Make sure that output directory is "
                                    "writable."));
    }
  }
}

TrayIcon* Application::trayIcon() {
  return m_guiNotifications->trayIcon();
}

QIcon Application::desktopAwareIcon() const {
  if (settings()->value(GROUP(GUI), SETTING(GUI::CustomColoredTrayIcon)).toBool() &&
      settings()->value(GROUP(GUI), SETTING(GUI::CustomColoredTrayIconAsAppIcon)).toBool()) {
    QColor background_color(settings()->value(GROUP(GUI), SETTING(GUI::CustomColoredTrayIconBackground)).toString());

    if (IconFactory::ensureCustomColoredIcons(background_color)) {
      QIcon custom_icon(IconFactory::customColoredAppIconPath());

      if (!custom_icon.isNull()) {
        return custom_icon;
      }
    }
  }

  auto from_theme = m_icons->fromTheme(QSL(APP_LOW_NAME));

  if (!from_theme.isNull()) {
    return from_theme;
  }
  else {
    return QIcon(APP_ICON_PATH);
  }
}

void Application::showTrayIcon() {
  m_guiNotifications->showTrayIcon();
}

void Application::deleteTrayIcon() {
  m_guiNotifications->deleteTrayIcon();
}

void Application::loadMessageToFeedAndArticleList(Feed* feed, const Message& message) {
  m_mainForm->display();
  m_mainForm->tabWidget()->feedMessageViewer()->loadMessageToFeedAndArticleList(feed, message);
}

void Application::showGuiMessage(Notification::Event event,
                                 const GuiMessage& msg,
                                 GuiMessageDestination dest,
                                 const GuiAction& action,
                                 QWidget* parent) {
  m_guiNotifications->showGuiMessage(event, msg, dest, action, parent);
}

WebViewer* Application::createWebView() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  if (cmdParser()->isSet(QSL(CLI_FORCETEXT_LONG))) {
    qDebugNN << LOGSEC_GUI << "Forcing QTextBrowser-based article viewer.";
    return new TextBrowserViewer();
  }
  else {
    return new WebEngineViewer();
  }
#else
  return new TextBrowserViewer();
#endif
}

bool Application::isWayland() const {
  return QGuiApplication::platformName() == QSL("wayland");
}

void Application::setupFont() {
  bool custom_font_enabled = qApp->settings()->value(GROUP(GUI), SETTING(GUI::CustomizeAppFont)).toBool();
  bool aa_enabled = qApp->settings()->value(GROUP(GUI), SETTING(GUI::FontAntialiasing)).toBool();

  QFont fon = QApplication::font();

  if (custom_font_enabled) {
    if (fon.fromString(settings()->value(GROUP(GUI), GUI::AppFont, fon.toString()).toString())) {
      qDebugNN << LOGSEC_CORE << "Loaded global custom font" << QUOTE_W_SPACE_DOT(fon.toString());
    }
    else {
      qWarningNN << LOGSEC_CORE << "Failed to load custom global font.";
    }
  }

  fon.setStyleStrategy(aa_enabled ? QFont::StyleStrategy::PreferAntialias
                                  : QFont::StyleStrategy(QFont::StyleStrategy::NoAntialias |
                                                         QFont::StyleStrategy::NoSubpixelAntialias));

  QApplication::setFont(fon);
}

void Application::setupWorkHorsePool() {
  auto ideal_th_count = QThread::idealThreadCount();
  int custom_threads = cmdParser()->value(QSL(CLI_THREADS)).toInt();
  int max_th_count = 0;

  // NOTE: Parenthesises are there to fix std::min build.
  if (custom_threads > 0) {
    max_th_count = (std::min)(MAX_THREADPOOL_THREADS, custom_threads);
  }
  else if (ideal_th_count > 1) {
    max_th_count = (std::min)(MAX_THREADPOOL_THREADS, 2 * ideal_th_count);
  }

  QThreadPool* pool;

#if QT_VERSION_MAJOR == 5
  pool = QThreadPool::globalInstance();
#else
  pool = m_workHorsePool;
#endif

  // NOTE: Qt 5 sadly does not allow to specify custom thread pool for
  // QtConcurrent::mapped() method, so we have to use global thread pool
  // there.
  if (max_th_count > 0) {
    pool->setMaxThreadCount(max_th_count);
  }

  // NOTE: Do not expire threads so that their IDs are not reused.
  // This fixes cross-thread QSqlDatabase access.
  pool->setExpiryTimeout(-1);

#if QT_VERSION >= 0x060200 // Qt >= 6.2.0
  // Avoid competing with interactive processes/threads by running the
  // worker pool at a very low priority
  pool->setThreadPriority(QThread::Priority::LowestPriority);
#endif
}

void Application::reloadCurrentSkin(bool replace_existing_qss) {
  m_skins->loadCurrentSkin(replace_existing_qss);
}

void Application::determineFirstRuns() {
  m_firstRunEver = settings()->value(GROUP(General), SETTING(General::FirstRun)).toBool();
  m_firstRunCurrentVersion =
    settings()->value(GROUP(General), QString(General::FirstRun) + QL1C('_') + APP_VERSION, true).toBool();

  eliminateFirstRuns();
}

void Application::parseCmdArgumentsFromOtherInstance(const QString& message) {
  m_commandLine->parseOtherInstanceArguments(message);
}

void Application::parseCmdArgumentsFromMyInstance(const QStringList& raw_cli_args, QString& custom_ua) {
  m_commandLine->parseMyArguments(raw_cli_args, custom_ua);
}

void Application::displayLog() {
  m_logManager->displayLog();
}

QString Application::customDataFolder() const {
  return m_paths->customDataFolder();
}

bool Application::event(QEvent* event) {
#if QT_VERSION_MAJOR >= 6 && defined(Q_OS_WIN)
  if (event->type() == QEvent::Type::Quit) {
    closeAllWindows();

    /*
    for (auto* w : topLevelWidgets()) {
      w->close();
    }
    */

    /*
    event->accept();
    return QCoreApplication::event(event);
    */
  }
#endif

  return QApplication::event(event);
}
