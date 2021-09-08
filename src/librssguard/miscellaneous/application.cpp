// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"

#include "3rd-party/boolinq/boolinq.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/toolbars/statusbar.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/notificationfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/standard/standardserviceroot.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <iostream>

#include <QProcess>
#include <QSessionManager>
#include <QSslSocket>
#include <QTimer>

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#if defined(USE_WEBENGINE)
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/networkurlinterceptor.h"

#include <QWebEngineDownloadItem>
#include <QWebEngineProfile>
#endif

Application::Application(const QString& id, int& argc, char** argv)
  : SingleApplication(id, argc, argv), m_updateFeedsLock(new Mutex()) {
  parseCmdArgumentsFromMyInstance();
  qInstallMessageHandler(performLogging);

  m_feedReader = nullptr;
  m_quitLogicDone = false;
  m_mainForm = nullptr;
  m_trayIcon = nullptr;
  m_settings = Settings::setupSettings(this);
  m_webFactory = new WebFactory(this);
  m_system = new SystemFactory(this);
  m_skins = new SkinFactory(this);
  m_localization = new Localization(this);
  m_icons = new IconFactory(this);
  m_database = new DatabaseFactory(this);
  m_downloadManager = nullptr;
  m_notifications = new NotificationFactory(this);
  m_shouldRestart = false;

  determineFirstRuns();

  //: Abbreviation of language, e.g. en.
  //: Use ISO 639-1 code here combined with ISO 3166-1 (alpha-2) code.
  //: Examples: "cs", "en", "it", "cs_CZ", "en_GB", "en_US".
  QObject::tr("LANG_ABBREV");

  //: Name of translator - optional.
  QObject::tr("LANG_AUTHOR");

  connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);
  connect(this, &Application::commitDataRequest, this, &Application::onCommitData);
  connect(this, &Application::saveStateRequest, this, &Application::onSaveState);

#if defined(Q_OS_LINUX)
  QString app_dir = QString::fromLocal8Bit(qgetenv("APPDIR"));

  if (!app_dir.isEmpty()) {
    bool success = qputenv("GST_PLUGIN_SYSTEM_PATH_1_0",
                           QSL("%1/usr/lib/gstreamer-1.0:%2").arg(app_dir,
                                                                  QString::fromLocal8Bit(qgetenv("GST_PLUGIN_SYSTEM_PATH_1_0"))).toLocal8Bit());
    success = qputenv("GST_PLUGIN_SCANNER_1_0",
                      QSL("%1/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner").arg(app_dir).toLocal8Bit()) && success;
    if (!success) {
      qWarningNN << LOGSEC_CORE << "Unable to set up GStreamer environment.";
    }
  }
#endif

#if defined(USE_WEBENGINE)
  m_webFactory->urlIinterceptor()->load();

  connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, &Application::downloadRequested);
  connect(m_webFactory->adBlock(), &AdBlockManager::processTerminated, this, &Application::onAdBlockFailure);

  QTimer::singleShot(3000, this, [=]() {
    try {
      m_webFactory->adBlock()->setEnabled(qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::AdBlockEnabled)).toBool());
    }
    catch (...) {
      onAdBlockFailure();
    }
  });
#endif

  m_webFactory->updateProxy();

  if (isFirstRun()) {
    m_notifications->save({
      Notification(Notification::Event::GeneralEvent, true),
      Notification(Notification::Event::NewUnreadArticlesFetched, true,
                   QSL("%1/notify.wav").arg(SOUNDS_BUILTIN_DIRECTORY)),
      Notification(Notification::Event::NewAppVersionAvailable, true),
      Notification(Notification::Event::LoginFailure, true)
    }, settings());
  }
  else {
    m_notifications->load(settings());
  }

  QTimer::singleShot(1000, system(), &SystemFactory::checkForUpdatesOnStartup);

  qDebugNN << LOGSEC_CORE
           << "OpenSSL version:"
           << QUOTE_W_SPACE_DOT(QSslSocket::sslLibraryVersionString());

  qDebugNN << LOGSEC_CORE
           << "OpenSSL supported:"
           << QUOTE_W_SPACE_DOT(QSslSocket::supportsSsl());
}

Application::~Application() {
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
      SystemTrayIcon::isSystemTrayDesired() &&
      SystemTrayIcon::isSystemTrayAreaAvailable()) {
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

void Application::showPolls() const {
  /*if (isFirstRunCurrentVersion()) {
     web()->openUrlInExternalBrowser(QSL("https://forms.gle/Son3h3xg2ZtCmi9K8"));
     }*/
}

void Application::offerChanges() const {
  if (isFirstRunCurrentVersion()) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         QSL(APP_NAME),
                         QObject::tr("Welcome to %1.\n\nPlease, check NEW stuff included in this\n"
                                     "version by clicking this popup notification.").arg(QSL(APP_LONG_NAME)),
                         QSystemTrayIcon::MessageIcon::NoIcon, {}, {}, tr("Go to changelog"), [] {
      FormAbout(qApp->mainForm()).exec();
    });
  }
}

bool Application::isAlreadyRunning() {
  return m_allowMultipleInstances
      ? false
      : sendMessage((QStringList() << QSL("-%1").arg(QSL(CLI_IS_RUNNING))
                                   << Application::arguments().mid(1)).join(QSL(ARGUMENTS_LIST_SEPARATOR)));
}

QStringList Application::builtinSounds() const {
  auto builtin_sounds = QDir(QSL(SOUNDS_BUILTIN_DIRECTORY)).entryInfoList(QDir::Filter::Files, QDir::SortFlag::Name);
  auto iter = boolinq::from(builtin_sounds).select([](const QFileInfo& i) {
    return i.absoluteFilePath();
  }).toStdList();
  auto descs = FROM_STD_LIST(QStringList, iter);

  return descs;
}

FeedReader* Application::feedReader() {
  return m_feedReader;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != nullptr && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();

#if defined(USE_WEBENGINE)
    m_userActions.append(m_webFactory->adBlock()->adBlockIcon());
#endif
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

NotificationFactory* Application::notifications() const {
  return m_notifications;
}

void Application::setFeedReader(FeedReader* feed_reader) {
  m_feedReader = feed_reader;

  connect(m_feedReader, &FeedReader::feedUpdatesFinished, this, &Application::onFeedUpdatesFinished);
  connect(m_feedReader->feedsModel(), &FeedsModel::messageCountsChanged, this, &Application::showMessagesNumber);
}

IconFactory* Application::icons() {
  return m_icons;
}

DownloadManager* Application::downloadManager() {
  if (m_downloadManager == nullptr) {
    m_downloadManager = new DownloadManager();
    connect(m_downloadManager, &DownloadManager::downloadFinished, mainForm()->statusBar(), &StatusBar::clearProgressDownload);
    connect(m_downloadManager, &DownloadManager::downloadProgressed, mainForm()->statusBar(), &StatusBar::showProgressDownload);
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
  return applicationDirPath() + QDir::separator() + QSL("data4");
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
#if defined(Q_OS_ANDROID)
  return IOFactory::getSystemFolder(QStandardPaths::GenericDataLocation) + QDir::separator() + QSL(APP_NAME) + QSL(" 4");
#else
  return configFolder() + QDir::separator() + QSL(APP_NAME) + QSL(" 4");
#endif
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

void Application::backupDatabaseSettings(bool backup_database, bool backup_settings,
                                         const QString& target_path, const QString& backup_name) {
  if (!QFileInfo(target_path).isWritable()) {
    throw ApplicationException(tr("Output directory is not writable."));
  }

  if (backup_settings) {
    settings()->sync();

    if (!IOFactory::copyFile(settings()->fileName(), target_path + QDir::separator() + backup_name + BACKUP_SUFFIX_SETTINGS)) {
      throw ApplicationException(tr("Settings file not copied to output directory successfully."));
    }
  }

  if (backup_database) {
    // We need to save the database first.
    database()->driver()->saveDatabase();
    database()->driver()->backupDatabase(target_path, backup_name);
  }
}

void Application::restoreDatabaseSettings(bool restore_database, bool restore_settings,
                                          const QString& source_database_file_path, const QString& source_settings_file_path) {
  if (restore_database) {
    if (!qApp->database()->driver()->initiateRestoration(source_database_file_path)) {
      throw ApplicationException(tr("Database restoration was not initiated. Make sure that output directory is writable."));
    }
  }

  if (restore_settings) {
    if (!qApp->settings()->initiateRestoration(source_settings_file_path)) {
      throw ApplicationException(tr("Settings restoration was not initiated. Make sure that output directory is writable."));
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
  auto from_theme = m_icons->fromTheme(APP_LOW_NAME);

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
#if !defined(Q_OS_LINUX)
    if (!SystemTrayIcon::isSystemTrayAreaAvailable()) {
      qWarningNN << LOGSEC_GUI << "Tray icon area is not available.";
      return;
    }
#endif

    qDebugNN << LOGSEC_GUI << "Showing tray icon.";
    trayIcon()->show();
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

void Application::showGuiMessage(Notification::Event event, const QString& title,
                                 const QString& message, QSystemTrayIcon::MessageIcon message_type, bool show_at_least_msgbox,
                                 QWidget* parent, const QString& functor_heading, std::function<void()> functor) {

  if (SystemTrayIcon::areNotificationsEnabled()) {
    auto notification = m_notifications->notificationForEvent(event);

    notification.playSound(this);

    if (SystemTrayIcon::isSystemTrayDesired() &&
        SystemTrayIcon::isSystemTrayAreaAvailable() &&
        notification.balloonEnabled()) {
      trayIcon()->showMessage(title, message, message_type, TRAY_ICON_BUBBLE_TIMEOUT, std::move(functor));

      return;
    }
  }

  if (show_at_least_msgbox) {
    // Tray icon or OSD is not available, display simple text box.
    MessageBox::show(parent == nullptr ? mainFormWidget() : parent, QMessageBox::Icon(message_type), title, message,
                     {}, {}, QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok, {}, functor_heading, functor);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Silencing GUI message:" << QUOTE_W_SPACE_DOT(message);
  }
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

  // Now, we can check if application should just quit or restart itself.
  if (m_shouldRestart) {
    finish();
    qDebugNN << LOGSEC_CORE << "Killing local peer connection to allow another instance to start.";

    if (QProcess::startDetached(QDir::toNativeSeparators(applicationFilePath()), {})) {
      qDebugNN << LOGSEC_CORE << "New application instance was started.";
    }
    else {
      qCriticalNN << LOGSEC_CORE << "New application instance was not started successfully.";
    }
  }
}

void Application::showMessagesNumber(int unread_messages, bool any_feed_has_unread_messages) {
  if (m_trayIcon != nullptr) {
    m_trayIcon->setNumber(unread_messages, any_feed_has_unread_messages);
  }

#if defined(Q_OS_LINUX)
  QDBusMessage signal = QDBusMessage::createSignal(QSL("/"),
                                                   QSL("com.canonical.Unity.LauncherEntry"),
                                                   QSL("Update"));

  signal << QSL("application://%1").arg(APP_DESKTOP_ENTRY_FILE);

  QVariantMap setProperty;

  setProperty.insert("count", qint64(unread_messages));
  setProperty.insert("count-visible", unread_messages > 0);

  signal << setProperty;

  QDBusConnection::sessionBus().send(signal);
#endif
}

void Application::restart() {
  m_shouldRestart = true;
  quit();
}

#if defined(USE_WEBENGINE)

void Application::downloadRequested(QWebEngineDownloadItem* download_item) {
  downloadManager()->download(download_item->url());
  download_item->cancel();
  download_item->deleteLater();
}

void Application::onAdBlockFailure() {
  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       tr("AdBlock needs to be configured"),
                       tr("AdBlock component is not configured properly."),
                       QSystemTrayIcon::MessageIcon::Critical,
                       true,
                       {},
                       tr("Configure now"),
                       [=]() {
    m_webFactory->adBlock()->showDialog();
  });

  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::AdBlockEnabled, false);
}

#endif

void Application::onFeedUpdatesFinished(const FeedDownloadResults& results) {
  if (!results.updatedFeeds().isEmpty()) {
    // Now, inform about results via GUI message/notification.
    qApp->showGuiMessage(Notification::Event::NewUnreadArticlesFetched,
                         tr("Unread articles fetched"),
                         results.overview(10),
                         QSystemTrayIcon::MessageIcon::NoIcon);
  }
}

void Application::setupCustomDataFolder(const QString& data_folder) {
  if (!QDir().mkpath(data_folder)) {
    qCriticalNN << LOGSEC_CORE
                << "Failed to create custom data path"
                << QUOTE_W_SPACE(data_folder)
                << "thus falling back to standard setup.";
    m_customDataFolder = QString();
    return;
  }

  // Disable single instance mode.
  m_allowMultipleInstances = true;

  // Save custom data folder.
  m_customDataFolder = data_folder;
}

void Application::determineFirstRuns() {
  m_firstRunEver = settings()->value(GROUP(General),
                                     SETTING(General::FirstRun)).toBool();
  m_firstRunCurrentVersion = settings()->value(GROUP(General),
                                               QString(General::FirstRun) + QL1C('_') + APP_VERSION,
                                               true).toBool();

  eliminateFirstRuns();
}

void Application::parseCmdArgumentsFromOtherInstance(const QString& message) {
  if (message.isEmpty()) {
    qDebugNN << LOGSEC_CORE << "No execution message received from other app instances.";
    return;
  }

  qDebugNN << LOGSEC_CORE
           << "Received"
           << QUOTE_W_SPACE(message)
           << "execution message.";

#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
  QStringList messages = message.split(QSL(ARGUMENTS_LIST_SEPARATOR), Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
  QStringList messages = message.split(QSL(ARGUMENTS_LIST_SEPARATOR), QString::SplitBehavior::SkipEmptyParts);
#endif

  QCommandLineParser cmd_parser;

  messages.prepend(qApp->applicationFilePath());

  cmd_parser.addOption(QCommandLineOption({ QSL(CLI_QUIT_INSTANCE) }));
  cmd_parser.addOption(QCommandLineOption({ QSL(CLI_IS_RUNNING) }));
  cmd_parser.addPositionalArgument(QSL("urls"),
                                   QSL("List of URL addresses pointing to individual online feeds which should be added."),
                                   QSL("[url-1 ... url-n]"));

  if (!cmd_parser.parse(messages)) {
    qCriticalNN << LOGSEC_CORE << cmd_parser.errorText();
  }

  if (cmd_parser.isSet(QSL(CLI_QUIT_INSTANCE))) {
    quit();
    return;
  }
  else if (cmd_parser.isSet(QSL(CLI_IS_RUNNING))) {
    showGuiMessage(Notification::Event::GeneralEvent,
                   QSL(APP_NAME),
                   tr("Application is already running."),
                   QSystemTrayIcon::MessageIcon::Information);
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
                     tr("Cannot add feed"),
                     tr("Feed cannot be added because there is no active account which can add feeds."),
                     QSystemTrayIcon::MessageIcon::Warning,
                     true);
    }
  }
}

void Application::parseCmdArgumentsFromMyInstance() {
  QCommandLineOption help({ QSL(CLI_HELP_SHORT), QSL(CLI_HELP_LONG) },
                          QSL("Displays overview of CLI."));
  QCommandLineOption version({ QSL(CLI_VER_SHORT), QSL(CLI_VER_LONG) },
                             QSL("Displays version of the application."));
  QCommandLineOption log_file({ QSL(CLI_LOG_SHORT), QSL(CLI_LOG_LONG) },
                              QSL("Write application debug log to file. Note that logging to file may slow application down."),
                              QSL("log-file"));
  QCommandLineOption custom_data_folder({ QSL(CLI_DAT_SHORT), QSL(CLI_DAT_LONG) },
                                        QSL("Use custom folder for user data and disable single instance application mode."),
                                        QSL("user-data-folder"));
  QCommandLineOption disable_singleinstance({ QSL(CLI_SIN_SHORT), QSL(CLI_SIN_LONG) },
                                            QSL("Allow running of multiple application instances."));
  QCommandLineOption disable_debug({ QSL(CLI_NDEBUG_SHORT), QSL(CLI_NDEBUG_LONG) },
                                   QSL("Completely disable stdout/stderr outputs."));

  m_cmdParser.addOptions({ help, version, log_file, custom_data_folder, disable_singleinstance, disable_debug });
  m_cmdParser.addPositionalArgument(QSL("urls"),
                                    QSL("List of URL addresses pointing to individual online feeds which should be added."),
                                    QSL("[url-1 ... url-n]"));
  m_cmdParser.setApplicationDescription(QSL(APP_NAME));

  if (!m_cmdParser.parse(QCoreApplication::arguments())) {
    qCriticalNN << LOGSEC_CORE << m_cmdParser.errorText();
  }

  s_customLogFile = m_cmdParser.value(QSL(CLI_LOG_SHORT));

  if (!m_cmdParser.value(QSL(CLI_DAT_SHORT)).isEmpty()) {
    auto data_folder = QDir::toNativeSeparators(m_cmdParser.value(QSL(CLI_DAT_SHORT)));

    qDebugNN << LOGSEC_CORE
             << "User wants to use custom directory for user data (and disable single instance mode):"
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

  if (m_cmdParser.isSet(QSL(CLI_NDEBUG_SHORT))) {
    s_disableDebug = true;
    qDebugNN << LOGSEC_CORE << "Disabling any stdout/stderr outputs.";
  }
}

QString Application::customDataFolder() const {
  return m_customDataFolder;
}
