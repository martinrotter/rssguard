// Needed for setting ini file format on Mac OS.
#ifdef Q_OS_MAC
#include <QSettings>
#endif

#include "core/defs.h"
#include "core/debugging.h"
#include "core/settings.h"
#include "gui/themefactory.h"
#include "gui/formmain.h"
#include "gui/formwelcome.h"
#include "qtsingleapplication/qtsingleapplication.h"


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
  qInstallMessageHandler(Debugging::debugHandler);

  // TODO: Finish implementation of QtSingleApplication into RSS Guard.
  // This primarily concerns slot in FormMain which reacts when application is launched
  // repeatedly. See 'trivial' example from QtSingleApplication source code for more
  // information.
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

  // Add an extra path for non-system icon themes.
  ThemeFactory::setupSearchPaths();

  // Load icon theme from settings.
  ThemeFactory::loadCurrentIconTheme();

  // These settings needs to be set before any QSettings object.
  QtSingleApplication::setApplicationName(APP_NAME);
  QtSingleApplication::setApplicationVersion(APP_VERSION);
  QtSingleApplication::setOrganizationName(APP_AUTHORS);
  QtSingleApplication::setOrganizationDomain(APP_URL);
  QtSingleApplication::setWindowIcon(QIcon(APP_ICON_PATH));

  // Instantiate main application window.
  FormMain window;

  if (Settings::getInstance()->value(APP_CFG_GEN, "first_start", true).toBool()) {
    Settings::getInstance()->setValue(APP_CFG_GEN, "first_start", false);
    FormWelcome(&window).exec();
  }

  // Display main window.
  window.show();

  // Setup single-instance behavior.
  application.setActivationWindow(&window, true);
  QObject::connect(&application, &QtSingleApplication::messageReceived,
                   &window, &FormMain::processExecutionMessage);

  // Enter global event loop.
  return QtSingleApplication::exec();
}
