// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"

#include "3rd-party/boolinq/boolinq.h"
#include "3rd-party/sqlite/sqlite3.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formlog.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/toolbars/statusbar.h"
#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/notificationfactory.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/standard/standardserviceroot.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <iostream>

#include <QLoggingCategory>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QSessionManager>
#include <QSslSocket>
#include <QThreadPool>
#include <QTimer>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#if defined(USE_WEBENGINE)
#include "gui/webviewers/webengine/webengineviewer.h" // WebEngine-based web browsing.
#include "network-web/webengine/networkurlinterceptor.h"

#if QT_VERSION_MAJOR == 6
#include <QWebEngineDownloadRequest>
#else
#include <QWebEngineDownloadItem>
#endif
#endif

#if defined(Q_OS_WIN)
#if QT_VERSION_MAJOR == 5
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#else
#include <QWindow>
#include <QtGui/qpa/qplatformwindow_p.h>
#endif
#endif

#if defined(Q_OS_WIN)
#include <ShObjIdl.h>

#if QT_VERSION_MAJOR == 5
#include <QtWinExtras/QtWin>
#endif
#endif

Application::Application(const QString& id, int& argc, char** argv, const QStringList& raw_cli_args)
  : SingleApplication(id, argc, argv), m_rawCliArgs(raw_cli_args), m_updateFeedsLock(new Mutex()) {
  QString custom_ua;

  parseCmdArgumentsFromMyInstance(raw_cli_args, custom_ua);

  qInstallMessageHandler(performLogging);

  m_feedReader = nullptr;
  m_quitLogicDone = false;
  m_mainForm = nullptr;
  m_logForm = nullptr;
  m_trayIcon = nullptr;
  m_settings = Settings::setupSettings(this);

#if defined(USE_WEBENGINE)
  if (!m_forcedNoWebEngine && qEnvironmentVariableIsEmpty("QTWEBENGINE_CHROMIUM_FLAGS")) {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            settings()->value(GROUP(Browser), SETTING(Browser::WebEngineChromiumFlags)).toString().toLocal8Bit());
  }
#endif

  m_localization = new Localization(this);

  m_localization->loadActiveLanguage();

  m_nodejs = new NodeJs(m_settings, this);
  m_workHorsePool = new QThreadPool(this);
  m_webFactory = new WebFactory(this);
  m_system = new SystemFactory(this);
  m_skins = new SkinFactory(this);
  m_icons = new IconFactory(this);
  m_database = new DatabaseFactory(this);
  m_downloadManager = nullptr;
  m_notifications = new NotificationFactory(this);
  m_shouldRestart = false;

#if defined(Q_OS_WIN)
  m_windowsTaskBar = nullptr;

  const GUID qIID_ITaskbarList4 = {0xc43dc798, 0x95d1, 0x4bea, {0x90, 0x30, 0xbb, 0x99, 0xe2, 0x98, 0x3a, 0x1a}};
  HRESULT task_result = CoCreateInstance(CLSID_TaskbarList,
                                         nullptr,
                                         CLSCTX_INPROC_SERVER,
                                         qIID_ITaskbarList4,
                                         reinterpret_cast<void**>(&m_windowsTaskBar));

  if (FAILED(task_result)) {
    qCriticalNN << LOGSEC_CORE << "Taskbar integration for Windows failed to initialize with HRESULT:"
                << QUOTE_W_SPACE_DOT(task_result);

    m_windowsTaskBar = nullptr;
  }
  else if (FAILED(m_windowsTaskBar->HrInit())) {
    qCriticalNN << LOGSEC_CORE << "Taskbar integration for Windows failed to initialize with inner HRESULT:"
                << QUOTE_W_SPACE_DOT(m_windowsTaskBar->HrInit());

    m_windowsTaskBar->Release();
    m_windowsTaskBar = nullptr;
  }
#endif

  determineFirstRuns();

  //: Abbreviation of language, e.g. en.
  //: Use ISO 639-1 code here combined with ISO 3166-1 (alpha-2) code.
  //: Examples: "cs", "en", "it", "cs_CZ", "en_GB", "en_US".
  QObject::tr("LANG_ABBREV");

  //: Name of translator - optional.
  QObject::tr("LANG_AUTHOR");

  // Add an extra path for non-system icon themes and set current icon theme
  // and skin.
  m_icons->setupSearchPaths();
  m_icons->loadCurrentIconTheme();
  m_skins->loadCurrentSkin();

  connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);
  connect(this, &Application::commitDataRequest, this, &Application::onCommitData);
  connect(this, &Application::saveStateRequest, this, &Application::onSaveState);

  connect(m_nodejs, &NodeJs::packageError, this, &Application::onNodeJsPackageUpdateError);
  connect(m_nodejs, &NodeJs::packageInstalledUpdated, this, &Application::onNodeJsPackageInstalled);

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

  m_webFactory->setCustomUserAgent(custom_ua);

#if defined(USE_WEBENGINE)
  m_webFactory->urlIinterceptor()->load();

  const QString web_data_root = userDataFolder() + QDir::separator() + QSL("web");

  m_webFactory->engineProfile()->setCachePath(web_data_root + QDir::separator() + QSL("cache"));
  m_webFactory->engineProfile()->setHttpCacheType(QWebEngineProfile::HttpCacheType::DiskHttpCache);
  m_webFactory->engineProfile()->setPersistentStoragePath(web_data_root + QDir::separator() + QSL("storage"));

  m_webFactory->loadCustomCss(userDataFolder() + QDir::separator() + QSL("web") + QDir::separator() +
                              QSL("user-styles.css"));

  if (custom_ua.isEmpty()) {
    m_webFactory->engineProfile()->setHttpUserAgent(QString(HTTP_COMPLETE_USERAGENT));
  }
  else {
    m_webFactory->engineProfile()->setHttpUserAgent(custom_ua);
  }

  qDebugNN << LOGSEC_NETWORK << "Persistent web data storage path:"
           << QUOTE_W_SPACE_DOT(m_webFactory->engineProfile()->persistentStoragePath());

  connect(m_webFactory->engineProfile(), &QWebEngineProfile::downloadRequested, this, &Application::downloadRequested);
#endif

  connect(m_webFactory->adBlock(), &AdBlockManager::processTerminated, this, &Application::onAdBlockFailure);

  QTimer::singleShot(3000, this, [=]() {
    try {
      m_webFactory->adBlock()
        ->setEnabled(qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::AdBlockEnabled)).toBool());
    }
    catch (...) {
      onAdBlockFailure();
    }
  });

  m_webFactory->updateProxy();

  if (isFirstRun()) {
    m_notifications->save({Notification(Notification::Event::GeneralEvent, true),
                           Notification(Notification::Event::NewUnreadArticlesFetched,
                                        true,
                                        QSL("%1/notify.wav").arg(SOUNDS_BUILTIN_DIRECTORY)),
                           Notification(Notification::Event::NewAppVersionAvailable, true),
                           Notification(Notification::Event::LoginFailure, true),
                           Notification(Notification::Event::NodePackageUpdated, true),
                           Notification(Notification::Event::NodePackageFailedToUpdate, true)},
                          settings());
  }
  else {
    m_notifications->load(settings());
  }

  QTimer::singleShot(1000, system(), &SystemFactory::checkForUpdatesOnStartup);

  setupWorkHorsePool();

  qDebugNN << LOGSEC_CORE << "SQLite version:" << QUOTE_W_SPACE_DOT(SQLITE_VERSION);
  qDebugNN << LOGSEC_CORE << "OpenSSL version:" << QUOTE_W_SPACE_DOT(QSslSocket::sslLibraryVersionString());
  qDebugNN << LOGSEC_CORE << "OpenSSL supported:" << QUOTE_W_SPACE_DOT(QSslSocket::supportsSsl());
  qDebugNN << LOGSEC_CORE << "Global thread pool has"
           << NONQUOTE_W_SPACE(QThreadPool::globalInstance()->maxThreadCount()) << "threads.";
}

Application::~Application() {
#if defined(Q_OS_WIN)
  if (m_windowsTaskBar != nullptr) {
    m_windowsTaskBar->Release();
  }
#endif

  // Disable logging into the form.
  m_logForm = nullptr;

  qDebugNN << LOGSEC_CORE << "Destroying Application instance.";
}

QString s_customLogFile = QString();
bool s_disableDebug = false;

void Application::performLogging(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
#ifndef QT_NO_DEBUG_OUTPUT
  QString console_message = qFormatLogMessage(type, context, msg);

  if (!s_disableDebug) {
    std::cerr << console_message.toStdString() << std::endl;
  }

  if (!s_customLogFile.isEmpty()) {
    QFile log_file(s_customLogFile);

    if (log_file.open(QFile::OpenModeFlag::Append | QFile::OpenModeFlag::Unbuffered)) {
      log_file.write(console_message.toUtf8());
      log_file.write(QSL("\r\n").toUtf8());
      log_file.close();
    }
  }

  if (qApp != nullptr) {
    qApp->displayLogMessageInDialog(console_message);
  }
  if (type == QtMsgType::QtFatalMsg) {
    qApp->exit(EXIT_FAILURE);
  }
#else
  Q_UNUSED(type)
  Q_UNUSED(context)
  Q_UNUSED(msg)
#endif
}

void Application::reactOnForeignNotifications() {
  connect(this, &Application::messageReceived, this, &Application::parseCmdArgumentsFromOtherInstance);
}

void Application::hideOrShowMainForm() {
  // Display main window.
  if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool() &&
      SystemTrayIcon::isSystemTrayDesired() && SystemTrayIcon::isSystemTrayAreaAvailable()) {
    qDebugNN << LOGSEC_CORE << "Hiding the main window when the application is starting.";
    mainForm()->switchVisibility(true);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Showing the main window when the application is starting.";
    mainForm()->show();
  }
}

void Application::loadDynamicShortcuts() {
  DynamicShortcuts::load(userActions());
}

void Application::offerPolls() const {
  if (isFirstRunCurrentVersion()) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("%1 survey").arg(QSL(APP_NAME)),
                          tr("Please, fill the survey."),
                          QSystemTrayIcon::MessageIcon::Warning},
                         {false, true, false},
                         {tr("Go to survey"), [] {
                            qApp->web()->openUrlInExternalBrowser(QSL("https://docs.google.com/forms/d/e/"
                                                                      "1FAIpQLScQ_r_EwM6qojPsIMQHGdnSktU-WGHgporN69mpU-"
                                                                      "Tvq8y7XQ/viewform?usp=sf_link"));
                          }});
  }
}

void Application::offerChanges() const {
  if (isFirstRunCurrentVersion()) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Welcome"),
                          tr("Welcome to %1.\n\nPlease, check NEW stuff included in this\n"
                             "version by clicking this popup notification.")
                            .arg(QSL(APP_LONG_NAME)),
                          QSystemTrayIcon::MessageIcon::Information},
                         {},
                         {tr("Go to changelog"), [] {
                            FormAbout(true, qApp->mainForm()).exec();
                          }});
  }
}

bool Application::isAlreadyRunning() {
  return m_allowMultipleInstances
           ? false
           : sendMessage((QStringList() << QSL("-%1").arg(QSL(CLI_IS_RUNNING)) << Application::arguments().mid(1))
                           .join(QSL(ARGUMENTS_LIST_SEPARATOR)));
}

QStringList Application::builtinSounds() const {
  auto builtin_sounds = QDir(QSL(SOUNDS_BUILTIN_DIRECTORY)).entryInfoList(QDir::Filter::Files, QDir::SortFlag::Name);
  auto iter = boolinq::from(builtin_sounds)
                .select([](const QFileInfo& i) {
                  return i.absoluteFilePath();
                })
                .toStdList();
  auto descs = FROM_STD_LIST(QStringList, iter);

  return descs;
}

FeedReader* Application::feedReader() {
  return m_feedReader;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != nullptr && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();
    m_userActions.append(m_webFactory->adBlock()->adBlockIcon());
  }

  return m_userActions;
}

bool Application::isFirstRun() const {
  return m_firstRunEver;
}

bool Application::isFirstRunCurrentVersion() const {
  return m_firstRunCurrentVersion;
}

QCommandLineParser* Application::cmdParser() {
  return &m_cmdParser;
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

void Application::displayLogMessageInDialog(const QString& message) {
  if (m_logForm != nullptr && m_logForm->isVisible()) {
    emit sendLogToDialog(message);
  }
}

QThreadPool* Application::workHorsePool() const {
  return m_workHorsePool;
}

int Application::customAdblockPort() const {
  return m_customAdblockPort;
}

QStringList Application::rawCliArgs() const {
  return m_rawCliArgs;
}

#if defined(USE_WEBENGINE)
bool Application::forcedNoWebEngine() const {
  return m_forcedNoWebEngine;
}
#endif

NodeJs* Application::nodejs() const {
  return m_nodejs;
}

NotificationFactory* Application::notifications() const {
  return m_notifications;
}

void Application::setFeedReader(FeedReader* feed_reader) {
  m_feedReader = feed_reader;

  connect(m_feedReader, &FeedReader::feedUpdatesStarted, this, &Application::onFeedUpdatesStarted);
  connect(m_feedReader, &FeedReader::feedUpdatesProgress, this, &Application::onFeedUpdatesProgress);
  connect(m_feedReader, &FeedReader::feedUpdatesFinished, this, &Application::onFeedUpdatesFinished);
  connect(m_feedReader->feedsModel(), &FeedsModel::messageCountsChanged, this, &Application::showMessagesNumber);
}

IconFactory* Application::icons() {
  return m_icons;
}

DownloadManager* Application::downloadManager() {
  if (m_downloadManager == nullptr) {
    m_downloadManager = new DownloadManager();
    connect(m_downloadManager,
            &DownloadManager::downloadFinished,
            mainForm()->statusBar(),
            &StatusBar::clearProgressDownload);
    connect(m_downloadManager,
            &DownloadManager::downloadProgressed,
            mainForm()->statusBar(),
            &StatusBar::showProgressDownload);
  }

  return m_downloadManager;
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

void Application::setMainForm(FormMain* main_form) {
  m_mainForm = main_form;
}

QString Application::configFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::GenericConfigLocation);
}

QString Application::userDataAppFolder() const {
  // In "app" folder, we would like to separate all user data into own subfolder,
  // therefore stick to "data" folder in this mode.
  return QDir::toNativeSeparators(applicationDirPath() + QDir::separator() + QSL("data4"));
}

QString Application::userDataFolder() {
  if (settings()->type() == SettingsProperties::SettingsType::Custom) {
    return customDataFolder();
  }
  else if (settings()->type() == SettingsProperties::SettingsType::Portable) {
    return userDataAppFolder();
  }
  else {
    return userDataHomeFolder();
  }
}

QString Application::replaceDataUserDataFolderPlaceholder(QString text) const {
  auto user_data_folder = qApp->userDataFolder();

  return text.replace(QSL(USER_DATA_PLACEHOLDER), user_data_folder);
}

QStringList Application::replaceDataUserDataFolderPlaceholder(QStringList texts) const {
  auto user_data_folder = qApp->userDataFolder();

  return texts.replaceInStrings(QSL(USER_DATA_PLACEHOLDER), user_data_folder);
}

QString Application::userDataHomeFolder() const {
  QString pth;

#if defined(Q_OS_ANDROID)
  return pth = IOFactory::getSystemFolder(QStandardPaths::GenericDataLocation) + QDir::separator() + QSL(APP_NAME) +
               QSL(" 4");
#else
  return pth = configFolder() + QDir::separator() + QSL(APP_NAME) + QSL(" 4");
#endif

  return QDir::toNativeSeparators(pth);
}

QString Application::tempFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation);
}

QString Application::documentsFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::DocumentsLocation);
}

QString Application::homeFolder() const {
#if defined(Q_OS_ANDROID)
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::GenericDataLocation);
#else
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::HomeLocation);
#endif
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
    if (!qApp->database()->driver()->initiateRestoration(source_database_file_path)) {
      throw ApplicationException(tr("Database restoration was not initiated. Make sure that output directory is "
                                    "writable."));
    }
  }

  if (restore_settings) {
    if (!qApp->settings()->initiateRestoration(source_settings_file_path)) {
      throw ApplicationException(tr("Settings restoration was not initiated. Make sure that output directory is "
                                    "writable."));
    }
  }
}

SystemTrayIcon* Application::trayIcon() {
  if (m_trayIcon == nullptr) {
    if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::MonochromeTrayIcon)).toBool()) {
      m_trayIcon = new SystemTrayIcon(APP_ICON_MONO_PATH, APP_ICON_MONO_PLAIN_PATH, m_mainForm);
    }
    else {
      m_trayIcon = new SystemTrayIcon(APP_ICON_PATH, APP_ICON_PLAIN_PATH, m_mainForm);
    }

    connect(m_trayIcon, &SystemTrayIcon::shown, m_feedReader->feedsModel(), &FeedsModel::notifyWithCounts);
  }

  return m_trayIcon;
}

QIcon Application::desktopAwareIcon() const {
  auto from_theme = m_icons->fromTheme(QSL(APP_LOW_NAME));

  if (!from_theme.isNull()) {
    return from_theme;
  }
  else {
    return QIcon(APP_ICON_PATH);
  }
}

void Application::showTrayIcon() {
  // Display tray icon if it is enabled and available.
  if (SystemTrayIcon::isSystemTrayDesired()) {
    qDebugNN << LOGSEC_GUI << "User wants to have tray icon.";

    // Delay avoids race conditions and tray icon is properly displayed.
    qWarningNN << LOGSEC_GUI << "Showing tray icon with little delay.";

    QTimer::singleShot(
#if defined(Q_OS_WIN)
      500,
#else
      3000,
#endif
      this,
      [=]() {
        if (SystemTrayIcon::isSystemTrayAreaAvailable()) {
          qWarningNN << LOGSEC_GUI << "Tray icon is available, showing now.";
          trayIcon()->show();
        }
        else {
          m_feedReader->feedsModel()->notifyWithCounts();
        }

        // NOTE: Below things have to be performed after tray icon is (if enabled)
        // initialized.
        offerChanges();
        offerPolls();

#if defined(Q_OS_WIN)
#if QT_VERSION_MAJOR == 6
        // NOTE: Fixes https://github.com/martinrotter/rssguard/issues/953 for Qt 6.
        using QWindowsWindow = QNativeInterface::Private::QWindowsWindow;
        if (auto w_w = qApp->mainForm()->windowHandle()->nativeInterface<QWindowsWindow>()) {
          w_w->setHasBorderInFullScreen(true);
        }
#endif
#endif
      });
  }
  else {
    m_feedReader->feedsModel()->notifyWithCounts();
  }
}

void Application::deleteTrayIcon() {
  if (m_trayIcon != nullptr) {
    qDebugNN << LOGSEC_CORE << "Disabling tray icon, deleting it and raising main application window.";
    m_mainForm->display();
    delete m_trayIcon;
    m_trayIcon = nullptr;

    // Make sure that application quits when last window is closed.
    setQuitOnLastWindowClosed(true);
  }
}

void Application::showGuiMessageCore(Notification::Event event,
                                     const GuiMessage& msg,
                                     GuiMessageDestination dest,
                                     const GuiAction& action,
                                     QWidget* parent) {
  if (SystemTrayIcon::areNotificationsEnabled()) {
    auto notification = m_notifications->notificationForEvent(event);

    notification.playSound(this);

    if (SystemTrayIcon::isSystemTrayDesired() && SystemTrayIcon::isSystemTrayAreaAvailable() &&
        notification.balloonEnabled() && dest.m_tray) {
      trayIcon()->showMessage(msg.m_title.simplified().isEmpty() ? Notification::nameForEvent(notification.event())
                                                                 : msg.m_title,
                              msg.m_message,
                              msg.m_type,
                              TRAY_ICON_BUBBLE_TIMEOUT,
                              std::move(action.m_action));
      return;
    }
  }

  if (dest.m_messageBox || msg.m_type == QSystemTrayIcon::MessageIcon::Critical) {
    // Tray icon or OSD is not available, display simple text box.
    MsgBox::show(parent == nullptr ? mainFormWidget() : parent,
                 QMessageBox::Icon(msg.m_type),
                 msg.m_title,
                 msg.m_message,
                 {},
                 {},
                 QMessageBox::StandardButton::Ok,
                 QMessageBox::StandardButton::Ok,
                 {},
                 action.m_title,
                 action.m_action);
  }
  else if (dest.m_statusBar && mainForm()->statusBar() != nullptr && mainForm()->statusBar()->isVisible()) {
    mainForm()->statusBar()->showMessage(msg.m_message, 3000);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Silencing GUI message:" << QUOTE_W_SPACE_DOT(msg.m_message);
  }
}

void Application::showGuiMessage(Notification::Event event,
                                 const GuiMessage& msg,
                                 GuiMessageDestination dest,
                                 const GuiAction& action,
                                 QWidget* parent) {
  QMetaObject::invokeMethod(this,
                            "showGuiMessageCore",
                            Qt::ConnectionType::QueuedConnection,
                            Q_ARG(Notification::Event, event),
                            Q_ARG(const GuiMessage&, msg),
                            Q_ARG(GuiMessageDestination, dest),
                            Q_ARG(const GuiAction&, action),
                            Q_ARG(QWidget*, parent));
}

WebViewer* Application::createWebView() {
#if !defined(USE_WEBENGINE)
  return new TextBrowserViewer();
#else
  if (forcedNoWebEngine()) {
    return new TextBrowserViewer();
  }
  else {
    return new WebEngineViewer();
  }
#endif
}

void Application::onCommitData(QSessionManager& manager) {
  qDebugNN << LOGSEC_CORE << "OS asked application to commit its data.";

  onAboutToQuit();

  manager.setRestartHint(QSessionManager::RestartHint::RestartNever);
  manager.release();
}

void Application::onSaveState(QSessionManager& manager) {
  qDebugNN << LOGSEC_CORE << "OS asked application to save its state.";

  manager.setRestartHint(QSessionManager::RestartHint::RestartNever);
  manager.release();
}

void Application::onAboutToQuit() {
  if (m_quitLogicDone) {
    qWarningNN << LOGSEC_CORE << "On-close logic is already done.";
    return;
  }

  m_quitLogicDone = true;

  // Make sure that we obtain close lock BEFORE even trying to quit the application.
  const bool locked_safely = feedUpdateLock()->tryLock(4 * CLOSE_LOCK_TIMEOUT);

  processEvents();
  qDebugNN << LOGSEC_CORE << "Cleaning up resources and saving application state.";

  if (locked_safely) {
    // Application obtained permission to close in a safe way.
    qDebugNN << LOGSEC_CORE << "Close lock was obtained safely.";

    // We locked the lock to exit peacefully, unlock it to avoid warnings.
    feedUpdateLock()->unlock();
  }
  else {
    // Request for write lock timed-out. This means
    // that some critical action can be processed right now.
    qWarningNN << LOGSEC_CORE << "Close lock timed-out.";
  }

  qApp->feedReader()->quit();
  database()->driver()->saveDatabase();

  if (mainForm() != nullptr) {
    mainForm()->saveSize();
  }

  settings()->sync();

  // Now, we can check if application should just quit or restart itself.
  if (m_shouldRestart) {
    finish();
    qDebugNN << LOGSEC_CORE << "Killing local peer connection to allow another instance to start.";

    if (QProcess::startDetached(QDir::toNativeSeparators(applicationFilePath()), arguments().mid(1))) {
      qDebugNN << LOGSEC_CORE << "New application instance was started.";
    }
    else {
      qCriticalNN << LOGSEC_CORE << "New application instance was not started successfully.";
    }
  }
}

void Application::showMessagesNumber(int unread_messages, bool any_feed_has_new_unread_messages) {
  if (m_trayIcon != nullptr) {
    m_trayIcon->setNumber(unread_messages, any_feed_has_new_unread_messages);
  }

  // Set task bar overlay with number of unread articles.
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  // Use D-Bus "LauncherEntry" service on Linux.
  bool task_bar_count_enabled = settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();
  QDBusMessage signal = QDBusMessage::createSignal(QSL("/"), QSL("com.canonical.Unity.LauncherEntry"), QSL("Update"));

  signal << QSL("application://%1.desktop").arg(APP_REVERSE_NAME);

  QVariantMap setProperty;

  setProperty.insert("count", qint64(unread_messages));
  setProperty.insert("count-visible", task_bar_count_enabled && unread_messages > 0);

  signal << setProperty;

  QDBusConnection::sessionBus().send(signal);
#elif defined(Q_OS_WIN)
  // Use SetOverlayIcon Windows API method on Windows.
  bool task_bar_count_enabled = settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();

  if (m_mainForm != nullptr) {
    bool any_count = task_bar_count_enabled && unread_messages > 0;
    HRESULT overlay_result;

    if (any_count) {
      QImage overlay_icon = generateOverlayIcon(unread_messages);

#if QT_VERSION_MAJOR == 5
      HICON overlay_hicon = QtWin::toHICON(QPixmap::fromImage(overlay_icon));
#else
      HICON overlay_hicon = overlay_icon.toHICON();
#endif

      overlay_result =
        m_windowsTaskBar->SetOverlayIcon(reinterpret_cast<HWND>(m_mainForm->winId()), overlay_hicon, nullptr);

      DestroyIcon(overlay_hicon);
    }
    else {
      overlay_result = m_windowsTaskBar->SetOverlayIcon(reinterpret_cast<HWND>(m_mainForm->winId()), nullptr, nullptr);
    }

    if (FAILED(overlay_result)) {
      qCriticalNN << LOGSEC_GUI << "Failed to set overlay icon with HRESULT:" << QUOTE_W_SPACE_DOT(overlay_result);
    }
  }
  else {
    qCriticalNN << LOGSEC_GUI << "Main form not set for setting numbers.";
  }
#endif

  if (m_mainForm != nullptr) {
    m_mainForm->setWindowTitle(unread_messages > 0
                                 ? QSL("[%2] %1").arg(QSL(APP_LONG_NAME), QString::number(unread_messages))
                                 : QSL(APP_LONG_NAME));
  }
}

#if defined(Q_OS_WIN)
QImage Application::generateOverlayIcon(int number) const {
  QImage img(128, 128, QImage::Format::Format_ARGB32);
  QPainter p;
  QString num_txt;

  if (number < 1000) {
    num_txt = QString::number(number);
  }
  else if (number < 100000) {
    num_txt = QSL("%1k").arg(int(number / 1000));
  }
  else {
    num_txt = QChar(8734);
  }

  QPainterPath rounded_rectangle;
  rounded_rectangle.addRoundedRect(QRectF(img.rect()), 15, 15);
  QFont fon = font();

  if (num_txt.size() == 3) {
    fon.setPixelSize(img.width() * 0.52);
  }
  else if (num_txt.size() == 2) {
    fon.setPixelSize(img.width() * 0.68);
  }
  else {
    fon.setPixelSize(img.width() * 0.79);
  }

  p.begin(&img);
  p.setFont(fon);

  p.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::RenderHint::TextAntialiasing, true);

  img.fill(Qt::GlobalColor::transparent);

  p.fillPath(rounded_rectangle, Qt::GlobalColor::white);

  p.setPen(Qt::GlobalColor::black);
  p.drawPath(rounded_rectangle);

  p.drawText(img.rect().marginsRemoved(QMargins(0, 0, 0, img.height() * 0.05)),
             num_txt,
             QTextOption(Qt::AlignmentFlag::AlignCenter));
  p.end();

  return img;
}

#endif

void Application::restart() {
  m_shouldRestart = true;
  quit();
}

#if defined(USE_WEBENGINE)

#if QT_VERSION_MAJOR == 6
void Application::downloadRequested(QWebEngineDownloadRequest* download_item) {
#else
void Application::downloadRequested(QWebEngineDownloadItem* download_item) {
#endif
  downloadManager()->download(download_item->url());
  download_item->cancel();
  download_item->deleteLater();
}

#endif

void Application::onFeedUpdatesStarted() {
#if defined(Q_OS_WIN)
  // Use SetOverlayIcon Windows API method on Windows.
  bool task_bar_count_enabled = settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();

  if (task_bar_count_enabled && m_mainForm != nullptr && m_windowsTaskBar != nullptr) {
    m_windowsTaskBar->SetProgressValue(reinterpret_cast<HWND>(m_mainForm->winId()), 1ul, 100ul);
  }
#endif
}

void Application::onFeedUpdatesProgress(const Feed* feed, int current, int total) {
#if defined(Q_OS_WIN)
  // Use SetOverlayIcon Windows API method on Windows.
  bool task_bar_count_enabled = settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();

  if (task_bar_count_enabled && m_mainForm != nullptr && m_windowsTaskBar != nullptr) {
    m_windowsTaskBar->SetProgressValue(reinterpret_cast<HWND>(m_mainForm->winId()), current, total);
  }
#endif
}

void Application::onFeedUpdatesFinished(const FeedDownloadResults& results) {
  auto fds = results.updatedFeeds();
  bool some_unquiet_feed = boolinq::from(fds).any([](const QPair<Feed*, int>& fd) {
    return !fd.first->isQuiet();
  });

  if (some_unquiet_feed) {
    // Now, inform about results via GUI message/notification.
    qApp->showGuiMessage(Notification::Event::NewUnreadArticlesFetched,
                         {tr("Unread articles fetched"), results.overview(10), QSystemTrayIcon::MessageIcon::NoIcon});
  }

#if defined(Q_OS_WIN)
  // Use SetOverlayIcon Windows API method on Windows.
  bool task_bar_count_enabled = settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();

  if (task_bar_count_enabled && m_mainForm != nullptr && m_windowsTaskBar != nullptr) {
    m_windowsTaskBar->SetProgressState(reinterpret_cast<HWND>(m_mainForm->winId()), TBPFLAG::TBPF_NOPROGRESS);
  }
#endif
}

void Application::setupCustomDataFolder(const QString& data_folder) {
  if (!QDir().mkpath(data_folder)) {
    qCriticalNN << LOGSEC_CORE << "Failed to create custom data path" << QUOTE_W_SPACE(data_folder)
                << "thus falling back to standard setup.";
    m_customDataFolder = QString();
    return;
  }

  // Disable single instance mode.
  m_allowMultipleInstances = true;

  // Save custom data folder.
  m_customDataFolder = data_folder;
}

void Application::setupWorkHorsePool() {
  auto ideal_th_count = QThread::idealThreadCount();
  int custom_threads = m_cmdParser.value(QSL(CLI_THREADS)).toInt();

  if (custom_threads > 0) {
    m_workHorsePool->setMaxThreadCount((std::min)(MAX_THREADPOOL_THREADS, custom_threads));
  }
  else if (ideal_th_count > 1) {
    m_workHorsePool->setMaxThreadCount((std::min)(MAX_THREADPOOL_THREADS, 2 * ideal_th_count));
  }

  // NOTE: Do not expire threads so that their IDs are not reused.
  // This fixes cross-thread QSqlDatabase access.
  m_workHorsePool->setExpiryTimeout(-1);

#if QT_VERSION_MAJOR == 5
  // NOTE: Qt 5 sadly does not allow to specify custom thread pool for
  // QtConcurrent::mapped() method, so we have to use global thread pool
  // there.
  QThreadPool::globalInstance()->setMaxThreadCount(m_workHorsePool->maxThreadCount());
  QThreadPool::globalInstance()->setExpiryTimeout(m_workHorsePool->expiryTimeout());
#endif
}

void Application::onAdBlockFailure() {
  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       {tr("AdBlock needs to be configured"),
                        tr("AdBlock is not configured properly. Go to \"Settings\" -> \"Node.js\" and check "
                           "if your Node.js is properly configured."),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {true, true, false});

  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::AdBlockEnabled, false);
}

void Application::determineFirstRuns() {
  m_firstRunEver = settings()->value(GROUP(General), SETTING(General::FirstRun)).toBool();
  m_firstRunCurrentVersion =
    settings()->value(GROUP(General), QString(General::FirstRun) + QL1C('_') + APP_VERSION, true).toBool();

  eliminateFirstRuns();
}

void Application::parseCmdArgumentsFromOtherInstance(const QString& message) {
  if (message.isEmpty()) {
    qDebugNN << LOGSEC_CORE << "No execution message received from other app instances.";
    return;
  }

  qDebugNN << LOGSEC_CORE << "Received" << QUOTE_W_SPACE(message) << "execution message.";

#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
  QStringList messages = message.split(QSL(ARGUMENTS_LIST_SEPARATOR), Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
  QStringList messages = message.split(QSL(ARGUMENTS_LIST_SEPARATOR), QString::SplitBehavior::SkipEmptyParts);
#endif

  QCommandLineParser cmd_parser;

  messages.prepend(qApp->applicationFilePath());

  cmd_parser.addOption(QCommandLineOption({QSL(CLI_QUIT_INSTANCE)}));
  cmd_parser.addOption(QCommandLineOption({QSL(CLI_IS_RUNNING)}));

  fillCmdArgumentsParser(cmd_parser);

  if (!cmd_parser.parse(messages)) {
    qCriticalNN << LOGSEC_CORE << cmd_parser.errorText();
  }

  if (cmd_parser.isSet(QSL(CLI_QUIT_INSTANCE))) {
    quit();
    return;
  }
  else if (cmd_parser.isSet(QSL(CLI_IS_RUNNING))) {
    showGuiMessage(Notification::Event::GeneralEvent,
                   {tr("Already running"),
                    tr("Application is already running."),
                    QSystemTrayIcon::MessageIcon::Information});
    mainForm()->display();
  }

  messages = cmd_parser.positionalArguments();

  for (const QString& msg : qAsConst(messages)) {
    // Application was running, and someone wants to add new feed.
    ServiceRoot* rt = boolinq::from(feedReader()->feedsModel()->serviceRoots()).firstOrDefault([](ServiceRoot* root) {
      return root->supportsFeedAdding();
    });

    if (rt != nullptr) {
      rt->addNewFeed(nullptr, msg);
    }
    else {
      showGuiMessage(Notification::Event::GeneralEvent,
                     {tr("Cannot add feed"),
                      tr("Feed cannot be added because there is no active account which can add feeds."),
                      QSystemTrayIcon::MessageIcon::Warning});
    }
  }
}

void Application::parseCmdArgumentsFromMyInstance(const QStringList& raw_cli_args, QString& custom_ua) {
  fillCmdArgumentsParser(m_cmdParser);

  m_cmdParser.setApplicationDescription(QSL(APP_NAME));
  m_cmdParser.setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode::ParseAsLongOptions);

  if (!m_cmdParser.parse(raw_cli_args)) {
    qCriticalNN << LOGSEC_CORE << m_cmdParser.errorText();
  }

  s_customLogFile = m_cmdParser.value(QSL(CLI_LOG_SHORT));

  if (s_customLogFile.startsWith('\'')) {
    s_customLogFile = s_customLogFile.mid(1);
  }

  if (s_customLogFile.endsWith('\'')) {
    s_customLogFile.chop(1);
  }

  if (m_cmdParser.isSet(QSL(CLI_NDEBUG_SHORT))) {
    QLoggingCategory::setFilterRules(QSL("*.debug=false"));
  }

  if (!m_cmdParser.value(QSL(CLI_DAT_SHORT)).isEmpty()) {
    auto data_folder = QDir::toNativeSeparators(m_cmdParser.value(QSL(CLI_DAT_SHORT)));

    qDebugNN << LOGSEC_CORE << "User wants to use custom directory for user data (and disable single instance mode):"
             << QUOTE_W_SPACE_DOT(data_folder);

    setupCustomDataFolder(data_folder);
  }
  else {
    m_allowMultipleInstances = false;
  }

  if (m_cmdParser.isSet(QSL(CLI_HELP_SHORT))) {
    m_cmdParser.showHelp();
  }
  else if (m_cmdParser.isSet(QSL(CLI_VER_SHORT))) {
    m_cmdParser.showVersion();
  }

#if defined(USE_WEBENGINE)
  m_forcedNoWebEngine = m_cmdParser.isSet(QSL(CLI_FORCE_NOWEBENGINE_SHORT));

  if (m_forcedNoWebEngine) {
    qDebugNN << LOGSEC_CORE << "Forcing no-web-engine.";
  }
#endif

  if (m_cmdParser.isSet(QSL(CLI_SIN_SHORT))) {
    m_allowMultipleInstances = true;
    qDebugNN << LOGSEC_CORE << "Explicitly allowing this instance to run.";
  }

  if (m_cmdParser.isSet(QSL(CLI_NSTDOUTERR_SHORT))) {
    s_disableDebug = true;
    qDebugNN << LOGSEC_CORE << "Disabling any stdout/stderr outputs.";
  }

  if (!m_cmdParser.value(QSL(CLI_ADBLOCKPORT_SHORT)).isEmpty()) {
    m_customAdblockPort = m_cmdParser.value(QSL(CLI_ADBLOCKPORT_SHORT)).toInt();

    qDebugNN << LOGSEC_ADBLOCK << "Setting custom server port.";
  }
  else {
    m_customAdblockPort = 0;
  }

  custom_ua = m_cmdParser.value(QSL(CLI_USERAGENT_SHORT));
}

void Application::displayLog() {
  if (m_logForm == nullptr) {
    m_logForm = new FormLog(m_mainForm);

    connect(this,
            &Application::sendLogToDialog,
            m_logForm,
            &FormLog::appendLogMessage,
            Qt::ConnectionType::QueuedConnection);
  }

  m_logForm->close();
  m_logForm->show();
}

void Application::fillCmdArgumentsParser(QCommandLineParser& parser) {
  QCommandLineOption help({QSL(CLI_HELP_SHORT), QSL(CLI_HELP_LONG)}, QSL("Displays overview of CLI."));
  QCommandLineOption version({QSL(CLI_VER_SHORT), QSL(CLI_VER_LONG)}, QSL("Displays version of the application."));
  QCommandLineOption
    log_file({QSL(CLI_LOG_SHORT), QSL(CLI_LOG_LONG)},
             QSL("Write application debug log to file. Note that logging to file may slow application down."),
             QSL("log-file"));
  QCommandLineOption
    custom_data_folder({QSL(CLI_DAT_SHORT), QSL(CLI_DAT_LONG)},
                       QSL("Use custom folder for user data and disable single instance application mode."),
                       QSL("user-data-folder"));
  QCommandLineOption disable_singleinstance({QSL(CLI_SIN_SHORT), QSL(CLI_SIN_LONG)},
                                            QSL("Allow running of multiple application instances."));

#if defined(USE_WEBENGINE)
  QCommandLineOption force_nowebengine({QSL(CLI_FORCE_NOWEBENGINE_SHORT), QSL(CLI_FORCE_NOWEBENGINE_LONG)},
                                       QSL("Force usage of simpler text-based embedded web browser."));
#endif

  QCommandLineOption disable_only_debug({QSL(CLI_NDEBUG_SHORT), QSL(CLI_NDEBUG_LONG)},
                                        QSL("Disable just \"debug\" output."));
  QCommandLineOption disable_debug({QSL(CLI_NSTDOUTERR_SHORT), QSL(CLI_NSTDOUTERR_LONG)},
                                   QSL("Completely disable stdout/stderr outputs."));
  QCommandLineOption forced_style({QSL(CLI_STYLE_SHORT), QSL(CLI_STYLE_LONG)},
                                  QSL("Force some application style."),
                                  QSL("style-name"));

  QCommandLineOption custom_ua({QSL(CLI_USERAGENT_SHORT), QSL(CLI_USERAGENT_LONG)},
                               QSL("User custom User-Agent HTTP header for all network requests."),
                               QSL("user-agent"));
  QCommandLineOption
    adblock_port({QSL(CLI_ADBLOCKPORT_SHORT), QSL(CLI_ADBLOCKPORT_LONG)},
                 QSL("Use custom port for AdBlock server. It is highly recommended to use values higher than 1024."),
                 QSL("port"));

  QCommandLineOption custom_threads(QSL(CLI_THREADS),
                                    QSL("Specify number of threads. Note that number cannot be higher than %1.")
                                      .arg(MAX_THREADPOOL_THREADS),
                                    QSL("count"));

  parser.addOptions({
    help, version, log_file, custom_data_folder, disable_singleinstance, disable_only_debug, disable_debug,
#if defined(USE_WEBENGINE)
      force_nowebengine,
#endif
      forced_style, adblock_port, custom_ua, custom_threads
  });
  parser.addPositionalArgument(QSL("urls"),
                               QSL("List of URL addresses pointing to individual online feeds which should be added."),
                               QSL("[url-1 ... url-n]"));
}

void Application::onNodeJsPackageUpdateError(const QList<NodeJs::PackageMetadata>& pkgs, const QString& error) {
  qApp->showGuiMessage(Notification::Event::NodePackageFailedToUpdate,
                       {{},
                        tr("Packages %1 were NOT updated because of error: %2.")
                          .arg(NodeJs::packagesToString(pkgs), error),
                        QSystemTrayIcon::MessageIcon::Critical});
}

void Application::onNodeJsPackageInstalled(const QList<NodeJs::PackageMetadata>& pkgs, bool already_up_to_date) {
  if (!already_up_to_date) {
    qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                         {{},
                          tr("Packages %1 were updated.").arg(NodeJs::packagesToString(pkgs)),
                          QSystemTrayIcon::MessageIcon::Information});
  }
}

QString Application::customDataFolder() const {
  return m_customDataFolder;
}
