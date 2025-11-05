// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "miscellaneous/application.h"
#include "services/abstract/label.h"

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
#include <clocale>
#endif

#if defined(Q_OS_WIN)
#include <Windows.h>

#if QT_VERSION_MAJOR == 5
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#else
#include <QWindow>
#endif
#endif

#include <QSettings>
#include <QTextCodec>

int main(int argc, char* argv[]) {
#if defined(Q_OS_WIN)
  // NOTE: Attaches console on Windows so that when RSS Guard is launched from console, stderr and stdout are visible.
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
  }
#endif

  qSetMessagePattern(QSL("time=\"%{time process}\" type=\"%{type}\" -> %{message}"));

  // NOTE: https://github.com/martinrotter/rssguard/issues/1118
  // NOTE: https://bugreports.qt.io/browse/QTBUG-117612
  qputenv("QT_DISABLE_AUDIO_PREPARE", "1");

#if defined(Q_OS_WIN)
  // NOTE: Turn off dark mode detection on Windows.
  qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif

  // High DPI stuff.
#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
  auto high_dpi_existing_env = qgetenv("QT_ENABLE_HIGHDPI_SCALING");

  if (high_dpi_existing_env.isEmpty()) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
  }
#else
  qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

#if QT_VERSION_MAJOR <= 5
  QApplication::setAttribute(Qt::ApplicationAttribute::AA_UseHighDpiPixmaps);
  QApplication::setAttribute(Qt::ApplicationAttribute::AA_EnableHighDpiScaling);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  QGuiApplication::setDesktopFileName(APP_REVERSE_NAME);
#endif

  QSettings::setDefaultFormat(QSettings::Format::IniFormat);
  QCoreApplication::setAttribute(Qt::ApplicationAttribute::AA_UseDesktopOpenGL, true);

  // We create our own "arguments" list as Qt strips something
  // sometimes out.
  char** const av = argv;
  QStringList raw_cli_args;

  raw_cli_args.reserve(argc);

  for (int a = 0; a < argc; a++) {
    raw_cli_args << QString::fromLocal8Bit(av[a]);
  }

#if QT_VERSION_MAJOR == 5
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

  // Set some names.
  QCoreApplication::setApplicationName(QSL(APP_NAME));
  QCoreApplication::setApplicationVersion(QSL(APP_VERSION));
  QCoreApplication::setOrganizationDomain(QSL(APP_URL));

  qDebugNN << LOGSEC_CORE << "Starting" << NONQUOTE_W_SPACE_DOT(APP_LONG_NAME);
  qDebugNN << LOGSEC_CORE << "Current UTC date/time is"
           << NONQUOTE_W_SPACE_DOT(QDateTime::currentDateTimeUtc().toString(Qt::DateFormat::ISODate));

  // Instantiate base application object.
  Application application(QSL(APP_LOW_NAME), argc, argv, raw_cli_args);

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
  qDebugNN << LOGSEC_CORE << "Setting locale for LC_NUMERIC to C as libmpv requires it.";
  std::setlocale(LC_NUMERIC, "C");
#endif

  qDebugNN << LOGSEC_CORE << "Instantiated class " << QUOTE_W_SPACE_DOT(application.metaObject()->className());

  // Check if another instance is running.
  if (application.isAlreadyRunning()) {
    qWarningNN << LOGSEC_CORE << "Another instance of the application is already running. Notifying it.";
    return EXIT_FAILURE;
  }

  qApp->setFeedReader(new FeedReader(&application));

  // Register needed metatypes.
  qRegisterMetaType<QNetworkAccessManager::Operation>("QNetworkAccessManager::Operation");
  qRegisterMetaType<QList<Message>>("QList<Message>");
  qRegisterMetaType<QList<RootItem*>>("QList<RootItem*>");
  qRegisterMetaType<QList<Label*>>("QList<Label*>");
  qRegisterMetaType<Label*>("Label*");

  // Set window icon, particularly for Linux/Wayland.
  QGuiApplication::setWindowIcon(qApp->desktopAwareIcon());

  qApp->reactOnForeignNotifications();

#if defined(Q_OS_WIN)
#if QT_VERSION_MAJOR == 5
  QWindowsWindowFunctions::setHasBorderInFullScreenDefault(true);

  // NOTE: https://github.com/martinrotter/rssguard/issues/953 for Qt 5.
  QWindowsWindowFunctions::setWindowActivationBehavior(QWindowsWindowFunctions::AlwaysActivateWindow);
#endif
#endif

  FormMain main_window;

  qApp->loadDynamicShortcuts();
  qApp->hideOrShowMainForm();
  qApp->feedReader()->loadSavedMessageFilters();
  qApp->feedReader()->feedsModel()->loadActivatedServiceAccounts();
  qApp->showTrayIcon();

  main_window.tabWidget()->feedMessageViewer()->respondToMainWindowResizes();
  main_window.tabWidget()->feedMessageViewer()->feedsView()->loadAllExpandStates();

  qApp
    ->parseCmdArgumentsFromOtherInstance(qApp->cmdParser()->positionalArguments().join(QSL(ARGUMENTS_LIST_SEPARATOR)));

  return Application::exec();
}
