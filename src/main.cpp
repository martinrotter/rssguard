// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "core/defs.h"
#include "core/databasefactory.h"
#include "core/debugging.h"
#include "core/localization.h"
#include "core/settings.h"
#include "core/dynamicshortcuts.h"
#include "gui/iconthemefactory.h"
#include "gui/skinfactory.h"
#include "gui/formmain.h"
#include "gui/formwelcome.h"
#include "gui/systemtrayicon.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "qtsingleapplication/qtsingleapplication.h"

// Needed for setting ini file format on Mac OS.
#ifdef Q_OS_MAC
#include <QSettings>
#endif

#include <QThread>
#include <QTranslator>
#include <QDebug>


int main(int argc, char *argv[]) {
  //: Name of language, e.g. English.
  QObject::tr("LANG_NAME");
  //: Abbreviation of language, e.g. en.
  //: Use ISO 639-1 code here combined with ISO 3166-1 (alpha-2) code.
  //: Examples: "cs_CZ", "en_GB", "en_US".
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

  // Add an extra path for non-system icon themes and set current icon theme
  // and skin.
  IconThemeFactory::instance()->setupSearchPaths();
  IconThemeFactory::instance()->loadCurrentIconTheme();
  SkinFactory::instance()->loadCurrentSkin();

  // Load localization and setup locale before any widget is constructed.
  Localization::instance()->load();

  // These settings needs to be set before any QSettings object.
  QtSingleApplication::setApplicationName(APP_NAME);
  QtSingleApplication::setApplicationVersion(APP_VERSION);
  QtSingleApplication::setOrganizationName(APP_AUTHOR);
  QtSingleApplication::setOrganizationDomain(APP_URL);
  QtSingleApplication::setWindowIcon(QIcon(APP_ICON_PATH));

  qDebug().nospace() << "Creating main application form in thread: \'" <<
                        QThread::currentThreadId() << "\'.";

  // Instantiate main application window.
  FormMain main_window;

  // Set correct information for main window.
  main_window.setWindowTitle(APP_LONG_NAME);

  // Now is a good time to initialize dynamic keyboard shortcuts.
  DynamicShortcuts::load(main_window.allActions());

  // Display welcome dialog if application is launched for the first time.
  if (Settings::instance()->value(APP_CFG_GEN, "first_start", true).toBool()) {
    Settings::instance()->setValue(APP_CFG_GEN, "first_start", false);
    FormWelcome(&main_window).exec();
  }

  // Display main window.
  if (Settings::instance()->value(APP_CFG_GUI, "start_hidden",
                                     false).toBool() &&
      SystemTrayIcon::isSystemTrayActivated()) {
    qDebug("Hiding the main window when the application is starting.");
    main_window.hide();
  }
  else {
    qDebug("Showing the main window when the application is starting.");
    main_window.show();
  }

  // Display tray icon if it is enabled and available.
  if (SystemTrayIcon::isSystemTrayActivated()) {
    SystemTrayIcon::instance()->show();
    main_window.tabWidget()->feedMessageViewer()->feedsView()->notifyWithCounts();
  }

  // Setup single-instance behavior.
  QObject::connect(&application, SIGNAL(messageReceived(QString)),
                   &main_window, SLOT(processExecutionMessage(QString)));

  // Enter global event loop.
  return QtSingleApplication::exec();
}
