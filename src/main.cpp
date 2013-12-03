// Needed for setting ini file format on Mac OS.
#ifdef Q_OS_MAC
#include <QSettings>
#endif

#include <QTranslator>

#include "core/databasefactory.h"
#include "core/defs.h"
#include "core/debugging.h"
#include "core/localization.h"
#include "core/settings.h"
#include "core/dynamicshortcuts.h"
#include "gui/iconthemefactory.h"
#include "gui/skinfactory.h"
#include "gui/formmain.h"
#include "gui/formwelcome.h"
#include "gui/systemtrayicon.h"
#include "qtsingleapplication/qtsingleapplication.h"



// TODO: Check if extra UNIX signalling is needed.
// Use <csignal> header for it - signal function and catch SIGHUP
// void my_terminate (int param) {
//   qApp->quit();
// }

int main(int argc, char *argv[]) {
  //: Name of language, e.g. English.
  QObject::tr("LANG_NAME");
  //: Abbreviation of language, e.g. en.
  //: Use ISO 639-1 code here!
  QObject::tr("LANG_ABBREV");
  //: Version of your translation, e.g. 1.0.
  QObject::tr("LANG_VERSION");
  //: Name of translator - optional.
  QObject::tr("LANG_AUTHOR");
  //: Email of translator - optional.
  QObject::tr("LANG_EMAIL");

  // Ensure that ini format is used as application settings storage on Mac OS.
#ifdef Q_OS_MAC
  QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

  // Setup debug output system.
#if QT_VERSION >= 0x050000
  qInstallMessageHandler(Debugging::debugHandler);
#else
  qInstallMsgHandler(Debugging::debugHandler);
#endif

  // Instantiate base application object.
  QtSingleApplication application(argc, argv);
  qDebug("Instantiated QtSingleApplication class.");

  // Check if another instance is running.
  if (application.sendMessage(APP_IS_RUNNING)) {
    qDebug("Another instance of the application is already running. Notifying it.");
    return EXIT_SUCCESS;
  }

  // Add 3rd party plugin directory to application PATH variable.
  // This is useful for styles, encoders, ...
  // This is probably not needed on Windows or Linux, not sure about Mac OS X.
#if defined(Q_OS_MAC)
  QtSingleApplication::addLibraryPath(APP_PLUGIN_PATH);
#endif

  // Add an extra path for non-system icon themes and set current icon theme
  // and skin.
  IconThemeFactory::getInstance()->setupSearchPaths();
  IconThemeFactory::getInstance()->loadCurrentIconTheme();
  SkinFactory::getInstance()->loadCurrentSkin();

  // Load localization and setup locale before any widget is constructed.
  LoadLocalization();

  // These settings needs to be set before any QSettings object.
  QtSingleApplication::setApplicationName(APP_NAME);
  QtSingleApplication::setApplicationVersion(APP_VERSION);
  QtSingleApplication::setOrganizationName(APP_AUTHORS);
  QtSingleApplication::setOrganizationDomain(APP_URL);
  QtSingleApplication::setWindowIcon(QIcon(APP_ICON_PATH));

  // Instantiate main application window.
  FormMain window;

  // Set correct information for main window.
  window.setWindowTitle(APP_LONG_NAME);

  // Now is a good time to initialize dynamic keyboard shortcuts.
  DynamicShortcuts::load(FormMain::getInstance()->getActions());

  // Display welcome dialog if application is launched for the first time.
  if (Settings::getInstance()->value(APP_CFG_GEN, "first_start", true).toBool()) {
    Settings::getInstance()->setValue(APP_CFG_GEN, "first_start", false);
    FormWelcome(&window).exec();
  }

  // Display main window.
  if (Settings::getInstance()->value(APP_CFG_GUI, "start_hidden",
                                     false).toBool() &&
      SystemTrayIcon::isSystemTrayActivated()) {
    qDebug("Hiding the main window when the application is starting.");
    window.hide();
  }
  else {
    qDebug("Showing the main window when the application is starting.");
    window.show();
  }

  // Display tray icon if it is enabled and available.
  if (SystemTrayIcon::isSystemTrayActivated()) {
    SystemTrayIcon::getInstance()->show();
  }

  // Setup single-instance behavior.
  QObject::connect(&application, SIGNAL(messageReceived(const QString&)),
                   &window, SLOT(processExecutionMessage(QString)));

  // Enter global event loop.
  return QtSingleApplication::exec();
}
