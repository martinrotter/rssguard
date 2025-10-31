// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"

#include "3rd-party/boolinq/boolinq.h"

#if defined(SYSTEM_SQLITE3)
#include <sqlite3.h>
#else
#include "3rd-party/sqlite/sqlite3.h"
#endif

#include "core/feedsmodel.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formlog.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/messagesview.h"
#include "gui/notifications/toastnotificationsmanager.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "gui/toolbars/statusbar.h"
#include "gui/webviewers/qlitehtml/qlitehtmlarticleviewer.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/notificationfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"

#include <iostream>

#include <QLoggingCategory>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QSessionManager>
#include <QSslSocket>
#include <QThreadPool>
#include <QTimer>
#include <QVersionNumber>

#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
#include <QQuickWindow>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusMessage>
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

bool s_disableDebug = false;

Application::Application(const QString& id, int& argc, char** argv, const QStringList& raw_cli_args)
  : SingleApplication(id, argc, argv), m_rawCliArgs(raw_cli_args), m_updateFeedsLock(new Mutex()) {
#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
  // HACK: Force rendering system to use OpenGL backend.
#if QT_VERSION_MAJOR < 6
  QQuickWindow::setSceneGraphBackend(QSGRendererInterface::GraphicsApi::OpenGL);
#else
  QQuickWindow::setGraphicsApi(QSGRendererInterface::GraphicsApi::OpenGL);
#endif
#endif

  QString custom_ua;

  parseCmdArgumentsFromMyInstance(raw_cli_args, custom_ua);

  qInstallMessageHandler(performLogging);

  m_feedReader = nullptr;
  m_quitLogicDone = false;
  m_mainForm = nullptr;
  m_logForm = nullptr;
  m_trayIcon = nullptr;
  m_settings = Settings::setupSettings(this);
  m_localization = new Localization(this);

  m_localization->loadActiveLanguage();

  m_workHorsePool = new QThreadPool(this);
  m_webFactory = new WebFactory(this);
  m_system = new SystemFactory(this);
  m_skins = new SkinFactory(this);
  m_icons = new IconFactory(this);
  m_database = new DatabaseFactory(this);
  m_notifications = new NotificationFactory(this);
  m_toastNotifications =
    (!isWayland() && m_notifications->useToastNotifications()) ? new ToastNotificationsManager(this) : nullptr;

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

  // Add an extra path for non-system icon themes and set current icon theme
  // and skin.
  m_icons->setupSearchPaths();
  m_icons->loadCurrentIconTheme();

  reloadCurrentSkin(false);
  setupFont();

  if (m_toastNotifications != nullptr) {
    connect(m_toastNotifications,
            &ToastNotificationsManager::openingArticleInArticleListRequested,
            this,
            &Application::loadMessageToFeedAndArticleList);
  }

  connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);
  connect(this, &Application::commitDataRequest, this, &Application::onCommitData);
  connect(this, &Application::saveStateRequest, this, &Application::onSaveState);

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

  if (isFirstRun()) {
    m_notifications->save({Notification(Notification::Event::GeneralEvent, true),
                           Notification(Notification::Event::NewUnreadArticlesFetched,
                                        true,
                                        false,
                                        true,
                                        QSL("%1/notify.wav").arg(SOUNDS_BUILTIN_DIRECTORY)),
                           Notification(Notification::Event::NewAppVersionAvailable, true),
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

  qDebugNN << LOGSEC_CORE << "Platform:" << QUOTE_W_SPACE_DOT(QGuiApplication::platformName());
  qDebugNN << LOGSEC_CORE << "SQLite version:" << QUOTE_W_SPACE_DOT(SQLITE_VERSION);

#if QT_VERSION >= 0x060100 // Qt >= 6.1.0
  qDebugNN << LOGSEC_CORE << "OpenSSL backends:" << QUOTE_W_SPACE_DOT(QSslSocket::availableBackends());
#endif

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

void Application::updateCliDebugStatus() {
  if (m_cmdParser.isSet(QSL(CLI_NSTDOUTERR_SHORT)) ||
      settings()->value(GROUP(General), SETTING(General::DisableDebugOutput)).toBool()) {
    qWarningNN << LOGSEC_CORE << "Disabling any/some stdout/stderr outputs.";
    s_disableDebug = true;
  }
}

void Application::performLogging(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
#ifndef QT_NO_DEBUG_OUTPUT

  if (s_disableDebug && (type == QtMsgType::QtDebugMsg || type == QtMsgType::QtInfoMsg)) {
    return;
  }

  QString console_message = qFormatLogMessage(type, context, msg);

  std::cerr << console_message.toStdString() << std::endl;

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
  /*
  if (isFirstRunCurrentVersion()) {
    qApp->web()->openUrlInExternalBrowser(QSL("https://forms.gle/3CZm95W6vrBLfi5K9"));
  }
  */
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

ToastNotificationsManager* Application::toastNotifications() const {
  return m_toastNotifications;
}

QThreadPool* Application::workHorsePool() const {
  return m_workHorsePool;
}

QStringList Application::rawCliArgs() const {
  return m_rawCliArgs;
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

  if (m_toastNotifications != nullptr) {
    connect(m_toastNotifications,
            &ToastNotificationsManager::dataChangeNotificationTriggered,
            m_mainForm->tabWidget()->feedMessageViewer()->messagesView(),
            &MessagesView::reactOnExternalDataChange);
  }
}

QString Application::configFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::GenericConfigLocation);
}

QString Application::userDataAppFolder() const {
  static int major_version = QVersionNumber::fromString(QSL(APP_VERSION)).majorVersion();

  // In "app" folder, we would like to separate all user data into own subfolder,
  // therefore stick to "data" folder in this mode.
  return QDir::toNativeSeparators(applicationDirPath() + QDir::separator() + QSL("data%1").arg(major_version));
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

QString Application::replaceUserDataFolderPlaceholder(QString text, bool double_escape) const {
  auto user_data_folder = qApp->userDataFolder();

  return text.replace(QSL(USER_DATA_PLACEHOLDER),
                      double_escape ? user_data_folder.replace(QDir::separator(), QString(2, QDir::separator()))
                                    : user_data_folder);
}

QStringList Application::replaceUserDataFolderPlaceholder(QStringList texts) const {
  auto user_data_folder = qApp->userDataFolder();

  return texts.replaceInStrings(QSL(USER_DATA_PLACEHOLDER), user_data_folder);
}

QString Application::userDataHomeFolder() const {
  static int major_version = QVersionNumber::fromString(QSL(APP_VERSION)).majorVersion();

  QString pth = configFolder() + QDir::separator() + QSL(APP_LOW_NAME) + QString::number(major_version);

  return pth;
}

QString Application::tempFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation);
}

QString Application::documentsFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::DocumentsLocation);
}

QString Application::homeFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::HomeLocation);
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
      if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::ColoredBusyTrayIcon)).toBool()) {
        m_trayIcon = new SystemTrayIcon(APP_ICON_MONO_PATH, APP_ICON_PLAIN_PATH, m_mainForm);
      }
      else {
        m_trayIcon = new SystemTrayIcon(APP_ICON_MONO_PATH, APP_ICON_MONO_PLAIN_PATH, m_mainForm);
      }
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
  bool show_dialog = true;

  if (m_notifications->areNotificationsEnabled()) {
    auto notification = m_notifications->notificationForEvent(event);

    show_dialog = notification.dialogEnabled();

    if (notification.soundEnabled()) {
      notification.playSound(this);
    }

    if (notification.balloonEnabled() && dest.m_tray) {
      if (notification.event() == Notification::Event::ArticlesFetchingStarted && m_mainForm != nullptr &&
          m_mainForm->isActiveWindow() && m_mainForm->isVisible()) {
        // We do not need to display the notification because
        // user will see that fetching is running because
        // he will see progress bar.
        return;
      }

      if (m_toastNotifications != nullptr) {
        // Toasts are enabled.
        m_toastNotifications->showNotification(event, msg, action);
      }
      else if (SystemTrayIcon::isSystemTrayDesired() && SystemTrayIcon::isSystemTrayAreaAvailable()) {
        // Use tray icon balloons (which are implemented as native notifications on most systems.
        trayIcon()->showMessage(msg.m_title.simplified().isEmpty() ? Notification::nameForEvent(notification.event())
                                                                   : msg.m_title,
                                msg.m_message,
                                msg.m_type,
                                TRAY_ICON_BUBBLE_TIMEOUT,
                                std::move(action.m_action));
      }

      return;
    }
  }

  if (show_dialog && (dest.m_messageBox || msg.m_type == QSystemTrayIcon::MessageIcon::Critical)) {
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
    mainForm()->statusBar()->showMessage(msg.m_message, 20000);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Silencing GUI message:" << QUOTE_W_SPACE_DOT(msg.m_message);
  }
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
  return new QLiteHtmlArticleViewer();
  // return new TextBrowserViewer();
}

bool Application::isWayland() const {
  return QGuiApplication::platformName() == QSL("wayland");
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

  QCoreApplication::processEvents();
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

  try {
    database()->driver()->saveDatabase();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Error when saving DB:" << QUOTE_W_SPACE_DOT(ex.message());
  }

  if (mainForm() != nullptr) {
    mainForm()->saveSize();
  }

  settings()->sync();
}

void Application::showMessagesNumber(int unread_messages, bool any_feed_has_new_unread_messages) {
  if (m_trayIcon != nullptr) {
    m_trayIcon->setNumber(unread_messages, any_feed_has_new_unread_messages);
  }

  // Use Qt function to set "badge" number directly in some cases.
#if defined(Q_OS_MACOS) && QT_VERSION >= 0x060500 // Qt >= 6.5.0
  qApp->setBadgeNumber(unread_messages);
#else
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
#endif

  if (m_mainForm != nullptr) {
    m_mainForm->setWindowTitle((settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnWindow)).toBool() &&
                                unread_messages > 0)
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
  auto fds = results.updatedFeeds().keys();
  bool some_unquiet_feed = boolinq::from(fds).any([](Feed* fd) {
    return !fd->isQuiet();
  });

  if (some_unquiet_feed) {
    GuiMessage msg = {tr("Unread articles fetched"), QString(), QSystemTrayIcon::MessageIcon::NoIcon};

    if (m_toastNotifications != nullptr) {
      // Show custom and richer overview of updated feeds and articles.
      msg.m_feedFetchResults = results;
    }
    else {
      // Show simpler overview of updated feeds.
      msg.m_message = results.overview(10);
    }

    qApp->showGuiMessage(Notification::Event::NewUnreadArticlesFetched, msg);
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

#if QT_VERSION >= 0x060200 // Qt >= 6.2.0
  // Avoid competing with interactive processes/threads by running the
  // worker pool at a very low priority
  m_workHorsePool->setThreadPriority(QThread::Priority::LowestPriority);
#endif

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
  if (message.isEmpty()) {
    qDebugNN << LOGSEC_CORE << "No execution message received from other app instances.";
    return;
  }

  qDebugNN << LOGSEC_CORE << "Received" << QUOTE_W_SPACE(message) << "execution message.";

  QStringList messages = message.split(QSL(ARGUMENTS_LIST_SEPARATOR), SPLIT_BEHAVIOR::SkipEmptyParts);
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

  for (const QString& msg : std::as_const(messages)) {
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

  if (m_cmdParser.isSet(QSL(CLI_SIN_SHORT))) {
    m_allowMultipleInstances = true;
    qDebugNN << LOGSEC_CORE << "Explicitly allowing this instance to run.";
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
    custom_data_folder({QSL(CLI_DAT_SHORT), QSL(CLI_DAT_LONG)},
                       QSL("Use custom folder for user data and disable single instance application mode."),
                       QSL("user-data-folder"));
  QCommandLineOption disable_singleinstance({QSL(CLI_SIN_SHORT), QSL(CLI_SIN_LONG)},
                                            QSL("Allow running of multiple application instances."));
  QCommandLineOption disable_only_debug({QSL(CLI_NDEBUG_SHORT), QSL(CLI_NDEBUG_LONG)},
                                        QSL("Disable just \"debug\" output."));
  QCommandLineOption
    disable_debug({QSL(CLI_NSTDOUTERR_SHORT), QSL(CLI_NSTDOUTERR_LONG)},
                  QSL("Disable DEBUG stdout/stderr outputs but keep more serious warnings and errors."));
  QCommandLineOption forced_style({QSL(CLI_STYLE_SHORT), QSL(CLI_STYLE_LONG)},
                                  QSL("Force some application style."),
                                  QSL("style-name"));
  QCommandLineOption custom_ua({QSL(CLI_USERAGENT_SHORT), QSL(CLI_USERAGENT_LONG)},
                               QSL("User custom User-Agent HTTP header for all network requests. This option "
                                   "takes precedence over User-Agent set via application settings."),
                               QSL("user-agent"));
  QCommandLineOption custom_threads(QSL(CLI_THREADS),
                                    QSL("Specify number of threads. Note that number cannot be higher than %1.")
                                      .arg(MAX_THREADPOOL_THREADS),
                                    QSL("count"));

  parser.addOptions({help,
                     version,
                     custom_data_folder,
                     disable_singleinstance,
                     disable_only_debug,
                     disable_debug,
                     forced_style,
                     custom_ua,
                     custom_threads});
  parser.addPositionalArgument(QSL("urls"),
                               QSL("List of URL addresses pointing to individual online feeds which should be added."),
                               QSL("[url-1 ... url-n]"));
}

QString Application::customDataFolder() const {
  return QDir::toNativeSeparators(m_customDataFolder);
}
