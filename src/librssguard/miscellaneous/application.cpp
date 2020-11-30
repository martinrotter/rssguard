// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"

#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/statusbar.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/mutex.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/standard/standardserviceroot.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <iostream>

#include <QProcess>
#include <QSessionManager>

#if defined(USE_WEBENGINE)
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/networkurlinterceptor.h"
#include "network-web/rssguardschemehandler.h"
#include "network-web/urlinterceptor.h"

#include <QWebEngineDownloadItem>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineUrlScheme>
#endif

Application::Application(const QString& id, int& argc, char** argv)
  : QtSingleApplication(id, argc, argv), m_updateFeedsLock(new Mutex()) {
  parseCmdArguments();

#if defined(USE_WEBENGINE)
  m_urlInterceptor = new NetworkUrlInterceptor(this);
#endif

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
  m_shouldRestart = false;

  // Setup debug output system.
  qInstallMessageHandler(performLogging);
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

#if defined(USE_WEBENGINE)
  QWebEngineUrlScheme url_scheme(QByteArray(APP_LOW_NAME));

  url_scheme.setDefaultPort(QWebEngineUrlScheme::SpecialPort::PortUnspecified);
  url_scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
  url_scheme.setFlags(QWebEngineUrlScheme::Flag::LocalScheme |
                      QWebEngineUrlScheme::Flag::LocalAccessAllowed |
                      QWebEngineUrlScheme::Flag::ServiceWorkersAllowed |
                      QWebEngineUrlScheme::Flag::ContentSecurityPolicyIgnored);

  QWebEngineUrlScheme::registerScheme(url_scheme);

  connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, &Application::downloadRequested);

#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  QWebEngineProfile::defaultProfile()->setUrlRequestInterceptor(m_urlInterceptor);
#else
  QWebEngineProfile::defaultProfile()->setRequestInterceptor(m_urlInterceptor);
#endif

  m_urlInterceptor->loadSettings();

  QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(QByteArray(APP_LOW_NAME),
                                                               new RssGuardSchemeHandler(QWebEngineProfile::defaultProfile()));
#endif

  m_webFactory->updateProxy();
}

Application::~Application() {
  qDebugNN << LOGSEC_CORE << "Destroying Application instance.";
}

QString s_customLogFile = QString();

void Application::performLogging(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
#ifndef QT_NO_DEBUG_OUTPUT
  QString console_message = qFormatLogMessage(type, context, msg);
  std::cout << console_message.toStdString() << std::endl;

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
  connect(this, &Application::messageReceived, this, &Application::processExecutionMessage);
}

void Application::hideOrShowMainForm() {
  // Display main window.
  if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool() && SystemTrayIcon::isSystemTrayActivated()) {
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
  if (isFirstRunCurrentVersion()) {
    web()->openUrlInExternalBrowser(QSL("https://forms.gle/Son3h3xg2ZtCmi9K8"));
  }
}

void Application::offerChanges() const {
  if (isFirstRunCurrentVersion()) {
    qApp->showGuiMessage(QSL(APP_NAME), QObject::tr("Welcome to %1.\n\nPlease, check NEW stuff included in this\n"
                                                    "version by clicking this popup notification.").arg(APP_LONG_NAME),
                         QSystemTrayIcon::NoIcon, nullptr, false, [] {
      FormAbout(qApp->mainForm()).exec();
    });
  }
}

bool Application::isAlreadyRunning() {
  return m_allowMultipleInstances
      ? false
      : sendMessage((QStringList() << APP_IS_RUNNING << Application::arguments().mid(1)).join(ARGUMENTS_LIST_SEPARATOR));
}

FeedReader* Application::feedReader() {
  return m_feedReader;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != nullptr && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();

#if defined(USE_WEBENGINE)
    m_userActions.append(AdBlockManager::instance()->adBlockIcon());
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

void Application::setFeedReader(FeedReader* feed_reader) {
  m_feedReader = feed_reader;
  connect(m_feedReader, &FeedReader::feedUpdatesFinished, this, &Application::onFeedUpdatesFinished);
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
  return IOFactory::getSystemFolder(QStandardPaths::GenericConfigLocation);
}

QString Application::userDataAppFolder() const {
  // In "app" folder, we would like to separate all user data into own subfolder,
  // therefore stick to "data" folder in this mode.
  return applicationDirPath() + QDir::separator() + QSL("data");
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

QString Application::userDataHomeFolder() const {
  // Fallback folder.
  const QString home_folder = homeFolder() + QDir::separator() + QSL(APP_LOW_H_NAME) + QDir::separator() + QSL("data");

  if (QDir().exists(home_folder)) {
    return home_folder;
  }
  else {
#if defined (Q_OS_ANDROID)
    return IOFactory::getSystemFolder(QStandardPaths::GenericDataLocation) + QDir::separator() + QSL(APP_NAME);
#else
    return configFolder() + QDir::separator() + QSL(APP_NAME);
#endif
  }
}

QString Application::tempFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::TempLocation);
}

QString Application::documentsFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::DocumentsLocation);
}

QString Application::homeFolder() const {
#if defined (Q_OS_ANDROID)
  return IOFactory::getSystemFolder(QStandardPaths::GenericDataLocation);
#else
  return IOFactory::getSystemFolder(QStandardPaths::HomeLocation);
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

  if (backup_database &&
      (database()->activeDatabaseDriver() == DatabaseFactory::UsedDriver::SQLITE ||
       database()->activeDatabaseDriver() == DatabaseFactory::UsedDriver::SQLITE_MEMORY)) {
    // We need to save the database first.
    database()->saveDatabase();

    if (!IOFactory::copyFile(database()->sqliteDatabaseFilePath(),
                             target_path + QDir::separator() + backup_name + BACKUP_SUFFIX_DATABASE)) {
      throw ApplicationException(tr("Database file not copied to output directory successfully."));
    }
  }
}

void Application::restoreDatabaseSettings(bool restore_database, bool restore_settings,
                                          const QString& source_database_file_path, const QString& source_settings_file_path) {
  if (restore_database) {
    if (!qApp->database()->initiateRestoration(source_database_file_path)) {
      throw ApplicationException(tr("Database restoration was not initiated. Make sure that output directory is writable."));
    }
  }

  if (restore_settings) {
    if (!qApp->settings()->initiateRestoration(source_settings_file_path)) {
      throw ApplicationException(tr("Settings restoration was not initiated. Make sure that output directory is writable."));
    }
  }
}

void Application::processExecutionMessage(const QString& message) {
  qDebugNN << LOGSEC_CORE
           << "Received '"
           << message
           << "' execution message from another application instance.";

  const QStringList messages = message.split(ARGUMENTS_LIST_SEPARATOR);

  if (messages.contains(APP_QUIT_INSTANCE)) {
    quit();
  }
  else {
    for (const QString& msg : messages) {
      if (msg == APP_IS_RUNNING) {
        showGuiMessage(APP_NAME, tr("Application is already running."), QSystemTrayIcon::Information);
        mainForm()->display();
      }
      else if (msg.startsWith(QL1S(URI_SCHEME_FEED_SHORT))) {
        // Application was running, and someone wants to add new feed.
        StandardServiceRoot* root = qApp->feedReader()->feedsModel()->standardServiceRoot();

        if (root != nullptr) {
          root->checkArgumentForFeedAdding(msg);
        }
        else {
          showGuiMessage(tr("Cannot add feed"),
                         tr("Feed cannot be added because standard RSS/ATOM account is not enabled."),
                         QSystemTrayIcon::Warning, qApp->mainForm(),
                         true);
        }
      }
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
    connect(m_feedReader->feedsModel(), &FeedsModel::messageCountsChanged, m_trayIcon, &SystemTrayIcon::setNumber);
  }

  return m_trayIcon;
}

#if defined(USE_WEBENGINE)
NetworkUrlInterceptor* Application::urlIinterceptor() {
  return m_urlInterceptor;
}

#endif

void Application::showTrayIcon() {
  // Display tray icon if it is enabled and available.
  if (SystemTrayIcon::isSystemTrayActivated()) {
    qDebugNN << LOGSEC_CORE << "Showing tray icon.";
    trayIcon()->show();
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

void Application::showGuiMessage(const QString& title, const QString& message,
                                 QSystemTrayIcon::MessageIcon message_type, QWidget* parent,
                                 bool show_at_least_msgbox, std::function<void()> functor) {
  if (SystemTrayIcon::areNotificationsEnabled() && SystemTrayIcon::isSystemTrayActivated()) {
    trayIcon()->showMessage(title, message, message_type, TRAY_ICON_BUBBLE_TIMEOUT, std::move(functor));
  }
  else if (show_at_least_msgbox) {
    // Tray icon or OSD is not available, display simple text box.
    MessageBox::show(parent, QMessageBox::Icon(message_type), title, message);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Silencing GUI message: '" << message << "'.";
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

#if defined(USE_WEBENGINE)
  AdBlockManager::instance()->save();
#endif

  // Make sure that we obtain close lock BEFORE even trying to quit the application.
  const bool locked_safely = feedUpdateLock()->tryLock(4 * CLOSE_LOCK_TIMEOUT);

  processEvents();
  qDebugNN << LOGSEC_CORE << "Cleaning up resources and saving application state.";

#if defined(Q_OS_WIN)
  system()->removeTrolltechJunkRegistryKeys();
#endif

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
  database()->saveDatabase();

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

#endif

void Application::onFeedUpdatesFinished(const FeedDownloadResults& results) {
  if (!results.updatedFeeds().isEmpty()) {
    // Now, inform about results via GUI message/notification.
    qApp->showGuiMessage(tr("New messages downloaded"), results.overview(10), QSystemTrayIcon::MessageIcon::NoIcon,
                         nullptr, false);
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

void Application::parseCmdArguments() {
  QCommandLineOption log_file(QStringList() << CLI_LOG_SHORT << CLI_LOG_LONG,
                              "Write application debug log to file. Note that logging to file may slow application down.",
                              "log-file");
  QCommandLineOption custom_data_folder(QStringList() << CLI_DAT_SHORT << CLI_DAT_LONG,
                                        "Use custom folder for user data and disable single instance application mode.",
                                        "user-data-folder");
  QCommandLineOption disable_singleinstance(QStringList() << CLI_SIN_SHORT << CLI_SIN_LONG,
                                            "Allow running of multiple application instances.");

  m_cmdParser.addOptions({ log_file, custom_data_folder, disable_singleinstance });
  m_cmdParser.addHelpOption();
  m_cmdParser.addVersionOption();
  m_cmdParser.setApplicationDescription(APP_NAME);

  m_cmdParser.process(*this);

  s_customLogFile = m_cmdParser.value(CLI_LOG_SHORT);

  if (!m_cmdParser.value(CLI_DAT_SHORT).isEmpty()) {
    auto data_folder = QDir::toNativeSeparators(m_cmdParser.value(CLI_DAT_SHORT));

    qDebugNN << LOGSEC_CORE
             << "User wants to use custom directory for user data (and disable single instance mode):"
             << QUOTE_W_SPACE_DOT(data_folder);

    setupCustomDataFolder(data_folder);
  }
  else {
    m_allowMultipleInstances = false;
  }

  if (m_cmdParser.isSet(CLI_SIN_SHORT)) {
    m_allowMultipleInstances = true;
    qDebugNN << LOGSEC_CORE << "Explicitly allowing this instance to run.";
  }
}

QString Application::customDataFolder() const {
  return m_customDataFolder;
}
