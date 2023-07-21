// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "miscellaneous/application.h"
#include "services/abstract/label.h"

#if defined(Q_OS_WIN)
#if QT_VERSION_MAJOR == 5
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#else
#include <QWindow>
#include <QtGui/qpa/qplatformwindow_p.h>
#endif
#endif

#include <QTextCodec>

int main(int argc, char* argv[]) {
  qSetMessagePattern(QSL("time=\"%{time process}\" type=\"%{type}\" -> %{message}"));

  // High DPI stuff.
#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
  qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
#else
  qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

#if QT_VERSION_MAJOR <= 5
  QApplication::setAttribute(Qt::ApplicationAttribute::AA_UseHighDpiPixmaps);
  QApplication::setAttribute(Qt::ApplicationAttribute::AA_EnableHighDpiScaling);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  QApplication::setDesktopFileName(APP_REVERSE_NAME + QSL(".desktop"));
#endif

#if defined(QT_STATIC)
  // NOTE: Add all used resources here.
  Q_INIT_RESOURCE(icons);
  Q_INIT_RESOURCE(sql);
  Q_INIT_RESOURCE(rssguard);
#endif

  // Ensure that ini format is used as application settings storage on macOS.
  QSettings::setDefaultFormat(QSettings::IniFormat);

#if defined(Q_OS_MACOS)
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

  // We create our own "arguments" list as Qt strips something
  // sometimes out.
  char** const av = argv;
  QStringList raw_cli_args;

  for (int a = 0; a < argc; a++) {
    raw_cli_args << QString::fromLocal8Bit(av[a]);
  }

#if QT_VERSION_MAJOR == 5
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

  // Instantiate base application object.
  Application application(QSL(APP_LOW_NAME), argc, argv, raw_cli_args);

  qDebugNN << LOGSEC_CORE << "Starting" << NONQUOTE_W_SPACE_DOT(APP_LONG_NAME);
  qDebugNN << LOGSEC_CORE << "Instantiated class " << QUOTE_W_SPACE_DOT(application.metaObject()->className());

  // Check if another instance is running.
  if (application.isAlreadyRunning()) {
    qWarningNN << LOGSEC_CORE << "Another instance of the application is already running. Notifying it.";
    return EXIT_FAILURE;
  }

  // Load localization and setup locale before any widget is constructed.
  qApp->localization()->loadActiveLanguage();
  qApp->setFeedReader(new FeedReader(&application));

  // Register needed metatypes.
  qRegisterMetaType<QList<Message>>("QList<Message>");
  qRegisterMetaType<QList<RootItem*>>("QList<RootItem*>");
  qRegisterMetaType<QList<Label*>>("QList<Label*>");
  qRegisterMetaType<Label*>("Label*");

  // These settings needs to be set before any QSettings object.
  Application::setApplicationName(QSL(APP_NAME));
  Application::setApplicationVersion(QSL(APP_VERSION));
  Application::setOrganizationDomain(QSL(APP_URL));
  Application::setWindowIcon(qApp->desktopAwareIcon());

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

#if defined(Q_OS_WIN)
#if QT_VERSION_MAJOR == 6
  // NOTE: Fixes https://github.com/martinrotter/rssguard/issues/953 for Qt 6.
  using QWindowsWindow = QNativeInterface::Private::QWindowsWindow;
  if (auto w_w = main_window.windowHandle()->nativeInterface<QWindowsWindow>()) {
    w_w->setHasBorderInFullScreen(true);
  }
#endif
#endif

  return Application::exec();
}
