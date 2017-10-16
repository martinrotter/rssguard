// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "definitions/definitions.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formmain.h"
#include "gui/dialogs/formupdate.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/debugging.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"

// Needed for setting ini file format on Mac OS.
#ifdef Q_OS_MAC
#include <QSettings>
#endif

#include <QDebug>
#include <QHttpMultiPart>
#include <QThread>
#include <QTimer>
#include <QTranslator>

#if defined (Q_OS_MAC)
extern void disableWindowTabbing();

#endif

int main(int argc, char* argv[]) {
  for (int i = 0; i < argc; i++) {
    const QString str = QString::fromLocal8Bit(argv[i]);

    if (str == "-h") {
      qDebug("Usage: rssguard [OPTIONS]\n\n"
             "Option\t\tMeaning\n"
             "-h\t\tDisplays this help.");
      return EXIT_SUCCESS;
    }
  }

  //: Abbreviation of language, e.g. en.
  //: Use ISO 639-1 code here combined with ISO 3166-1 (alpha-2) code.
  //: Examples: "cs", "en", "it", "cs_CZ", "en_GB", "en_US".
  QObject::tr("LANG_ABBREV");

  //: Name of translator - optional.
  QObject::tr("LANG_AUTHOR");

  // Ensure that ini format is used as application settings storage on Mac OS.
  QSettings::setDefaultFormat(QSettings::IniFormat);

  // Setup debug output system.
  qInstallMessageHandler(Debugging::debugHandler);

  // Instantiate base application object.
  Application application(APP_LOW_NAME, argc, argv);

  qDebug("Instantiated Application class.");

  // Check if another instance is running.
  if (application.sendMessage((QStringList() << APP_IS_RUNNING << application.arguments().mid(1)).join(ARGUMENTS_LIST_SEPARATOR))) {
    qWarning("Another instance of the application is already running. Notifying it.");
    return EXIT_FAILURE;
  }

  // Load localization and setup locale before any widget is constructed.
  qApp->localization()->loadActiveLanguage();
  application.setFeedReader(new FeedReader(&application));
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#if defined (Q_OS_MAC)
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
  disableWindowTabbing();
#endif

  // Register needed metatypes.
  qRegisterMetaType<QList<Message>>("QList<Message>");
  qRegisterMetaType<QList<RootItem*>>("QList<RootItem*>");

  // Add an extra path for non-system icon themes and set current icon theme
  // and skin.
  qApp->icons()->setupSearchPaths();
  qApp->icons()->loadCurrentIconTheme();
  qApp->skins()->loadCurrentSkin();

  // These settings needs to be set before any QSettings object.
  Application::setApplicationName(APP_NAME);
  Application::setApplicationVersion(APP_VERSION);
  Application::setOrganizationDomain(APP_URL);
  Application::setWindowIcon(QIcon(APP_ICON_PATH));

  // Setup single-instance behavior.
  QObject::connect(&application, &Application::messageReceived, &application, &Application::processExecutionMessage);
  qDebug().nospace() << "Creating main application form in thread: \'" << QThread::currentThreadId() << "\'.";

  // Instantiate main application window.
  FormMain main_window;

  // Set correct information for main window.
  main_window.setWindowTitle(APP_LONG_NAME);

  // Now is a good time to initialize dynamic keyboard shortcuts.
  DynamicShortcuts::load(qApp->userActions());

  // Display main window.
  if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool() && SystemTrayIcon::isSystemTrayActivated()) {
    qDebug("Hiding the main window when the application is starting.");
    main_window.switchVisibility(true);
  }
  else {
    qDebug("Showing the main window when the application is starting.");
    main_window.show();
  }

  // Display tray icon if it is enabled and available.
  if (SystemTrayIcon::isSystemTrayActivated()) {
    qApp->showTrayIcon();
  }

  // Load activated accounts.
  qApp->feedReader()->feedsModel()->loadActivatedServiceAccounts();

  if (qApp->isFirstRun() || qApp->isFirstRun(APP_VERSION)) {
    qApp->showGuiMessage(QSL(APP_NAME), QObject::tr("Welcome to %1.\n\nPlease, check NEW stuff included in this\n"
                                                    "version by clicking this popup notification.").arg(APP_LONG_NAME),
                         QSystemTrayIcon::NoIcon, 0, false, [] {
      FormAbout(qApp->mainForm()).exec();
    });
  }
  else {
    qApp->showGuiMessage(QSL(APP_NAME), QObject::tr("Welcome to %1.").arg(APP_NAME), QSystemTrayIcon::NoIcon);
  }

  if (qApp->settings()->value(GROUP(General), SETTING(General::UpdateOnStartup)).toBool()) {
    QObject::connect(qApp->system(), &SystemFactory::updatesChecked, [](QPair<QList<UpdateInfo>, QNetworkReply::NetworkError> updates) {
      QObject::disconnect(qApp->system(), &SystemFactory::updatesChecked, nullptr, nullptr);

      if (!updates.first.isEmpty() && updates.second == QNetworkReply::NoError &&
          SystemFactory::isVersionNewer(updates.first.at(0).m_availableVersion, APP_VERSION)) {
        qApp->showGuiMessage(QObject::tr("New version available"),
                             QObject::tr("Click the bubble for more information."),
                             QSystemTrayIcon::Information, qApp->mainForm(), false,
                             [] {
          FormUpdate(qApp->mainForm()).exec();
        });
      }
    });
    qApp->system()->checkForUpdates();
  }

  qApp->showPolls();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->loadAllExpandStates();

  /*
     OAuth2Service * srv = new OAuth2Service(
     "https://accounts.google.com/o/oauth2/auth",
     "https://accounts.google.com/o/oauth2/token",
     "369069180494-j66bgeciouinec1eem7fhvj6qm0as7q3.apps.googleusercontent.com",
     "vppQtxrEeBkImiXcjGYl9NxZ",
     "https://mail.google.com/");

     srv->setRefreshToken("1/RKE3oohSoTHE54L0IPflvndK-DcI7l0of3lVdLa1Q9Q");
     QObject::connect(srv, &OAuth2Service::authCodeObtained, [](QString auth_code) {
     int a = 5;
     });
     QObject::connect(srv, &OAuth2Service::authFailed, []() {
     int a = 5;
     });
     QObject::connect(srv, &OAuth2Service::tokensReceived, [srv](QString acc, QString ref, int exp) {

     QHttpMultiPart* multi = new QHttpMultiPart(srv);
     QHttpPart p1;
     QHttpPart p2;

     multi->setContentType(QHttpMultiPart::ContentType::MixedType);
     p1.setRawHeader("Content-Type", "application/http");
     p2.setRawHeader("Content-Type", "application/http");
     p1.setBody("GET /gmail/v1/users/me/messages\r\n");
     p2.setBody("GET /gmail/v1/users/me/labels\r\n");
     multi->append(p1);
     multi->append(p2);
     QNetworkRequest req;

     auto bearer = srv->bearer();
     req.setRawHeader(QString("Authorization").toLocal8Bit(), bearer.toLocal8Bit());

     req.setUrl(QUrl::fromUserInput("https://www.googleapis.com/batch"));
     auto* repl = SilentNetworkAccessManager::instance()->post(req, multi);

     //req.setUrl(QUrl::fromUserInput("https://www.googleapis.com/gmail/v1/users/me/labels"));
     //auto* repl = SilentNetworkAccessManager::instance()->get(req);

     QObject::connect(repl, &QNetworkReply::finished, [repl]() {
      auto resp = repl->readAll();
      auto a = 8;

      IOFactory::writeTextFile("b.html", resp);
     });

     int a = 5;
     });
     QObject::connect(srv, &OAuth2Service::tokensRetrieveError, [](QString err, QString desc) {
     int a = 5;
     });
     srv->login(); */

  // Enter global event loop.
  return Application::exec();
}
