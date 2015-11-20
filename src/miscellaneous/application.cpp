// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "miscellaneous/application.h"

#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/mutex.h"
#include "gui/feedsview.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagebox.h"
#include "gui/statusbar.h"
#include "gui/dialogs/formmain.h"
#include "exceptions/applicationexception.h"
#include "adblock/adblockmanager.h"

#include "services/abstract/serviceroot.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <QSessionManager>
#include <QThread>
#include <QProcess>


Application::Application(const QString &id, int &argc, char **argv)
  : QtSingleApplication(id, argc, argv),
    m_updateFeedsLock(NULL), m_feedServices(QList<ServiceEntryPoint*>()), m_userActions(QList<QAction*>()), m_mainForm(NULL),
    m_trayIcon(NULL), m_settings(NULL), m_system(NULL), m_skins(NULL),
    m_localization(NULL), m_icons(NULL), m_database(NULL), m_downloadManager(NULL), m_shouldRestart(false),
    m_notification(NULL) {
  connect(this, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
  connect(this, SIGNAL(commitDataRequest(QSessionManager&)), this, SLOT(onCommitData(QSessionManager&)));
  connect(this, SIGNAL(saveStateRequest(QSessionManager&)), this, SLOT(onSaveState(QSessionManager&)));
}

Application::~Application() {
  delete m_updateFeedsLock;
  qDeleteAll(m_feedServices);
}

QList<ServiceEntryPoint*> Application::feedServices() {
  if (m_feedServices.isEmpty()) {
    // NOTE: All installed services create their entry points here.
    m_feedServices.append(new StandardServiceEntryPoint());
    m_feedServices.append(new TtRssServiceEntryPoint());
  }

  return m_feedServices;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != NULL && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();
  }

  return m_userActions;
}

bool Application::isFirstRun() {
  return settings()->value(GROUP(General), SETTING(General::FirstRun)).toBool();
}

bool Application::isFirstRun(const QString &version) {
  return settings()->value(GROUP(General), QString(General::FirstRun) + QL1C('_') + version, true).toBool();
}

void Application::eliminateFirstRun() {
  settings()->setValue(GROUP(General), General::FirstRun, false);
}

void Application::eliminateFirstRun(const QString &version) {
  settings()->setValue(GROUP(General), QString(General::FirstRun) + QL1C('_') + version, false);
}

IconFactory *Application::icons() {
  if (m_icons == NULL) {
    m_icons = new IconFactory(this);
  }

  return m_icons;
}

DownloadManager *Application::downloadManager() {
  if (m_downloadManager == NULL) {
    m_downloadManager = new DownloadManager();

    connect(m_downloadManager, SIGNAL(downloadFinished()), mainForm()->statusBar(), SLOT(clearProgressDownload()));
    connect(m_downloadManager, SIGNAL(downloadProgress(int,QString)), mainForm()->statusBar(), SLOT(showProgressDownload(int,QString)));
  }

  return m_downloadManager;
}

Mutex *Application::feedUpdateLock() {
  if (m_updateFeedsLock == NULL) {
    m_updateFeedsLock = new Mutex();
  }

  return m_updateFeedsLock;
}

void Application::backupDatabaseSettings(bool backup_database, bool backup_settings,
                                         const QString &target_path, const QString &backup_name) {
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
      (database()->activeDatabaseDriver() == DatabaseFactory::SQLITE ||
       database()->activeDatabaseDriver() == DatabaseFactory::SQLITE_MEMORY)) {
    // We need to save the database first.
    database()->saveDatabase();

    if (!IOFactory::copyFile(database()->sqliteDatabaseFilePath(), target_path + QDir::separator() + backup_name + BACKUP_SUFFIX_DATABASE)) {
      throw ApplicationException(tr("Database file not copied to output directory successfully."));
    }
  }
}

void Application::restoreDatabaseSettings(bool restore_database, bool restore_settings,
                                          const QString &source_database_file_path, const QString &source_settings_file_path) {
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

void Application::processExecutionMessage(const QString &message) {
  qDebug("Received '%s' execution message from another application instance.", qPrintable(message));

  if (message == APP_IS_RUNNING) {
    showGuiMessage(APP_NAME, tr("Application is already running."), QSystemTrayIcon::Information);
    mainForm()->display();
  }
  else if (message == APP_QUIT_INSTANCE) {
    quit();
  }
}

SystemTrayIcon *Application::trayIcon() {
  if (m_trayIcon == NULL) {
    m_trayIcon = new SystemTrayIcon(APP_ICON_PATH, APP_ICON_PLAIN_PATH, m_mainForm);
    connect(m_trayIcon, SIGNAL(shown()),
            m_mainForm->tabWidget()->feedMessageViewer()->feedsView()->sourceModel(), SLOT(notifyWithCounts()));
  }

  return m_trayIcon;
}

void Application::showTrayIcon() {
  qDebug("Showing tray icon.");
  trayIcon()->show();
}

void Application::deleteTrayIcon() {
  if (m_trayIcon != NULL) {
    qDebug("Disabling tray icon, deleting it and raising main application window.");
    m_mainForm->display();
    delete m_trayIcon;
    m_trayIcon = NULL;

    // Make sure that application quits when last window is closed.
    setQuitOnLastWindowClosed(true);
  }
}

void Application::showGuiMessage(const QString &title, const QString &message,
                                 QSystemTrayIcon::MessageIcon message_type, QWidget *parent,
                                 bool show_at_least_msgbox, const QIcon &custom_icon,
                                 QObject *invokation_target, const char *invokation_slot) {
  if (Notification::areNotificationsActivated()) {
    // Show OSD instead if tray icon bubble, depending on settings.
    if (custom_icon.isNull()) {
      notification()->notify(message, title, message_type, invokation_target, invokation_slot);
    }
    else {
      notification()->notify(message, title, custom_icon, invokation_target, invokation_slot);
    }
  }
  else if (SystemTrayIcon::isSystemTrayActivated()) {
    trayIcon()->showMessage(title, message, message_type, TRAY_ICON_BUBBLE_TIMEOUT, invokation_target, invokation_slot);
  }
  else if (show_at_least_msgbox) {
    // Tray icon or OSD is not available, display simple text box.
    MessageBox::show(parent, (QMessageBox::Icon) message_type, title, message);
  }
  else {
    qDebug("Silencing GUI message: '%s'.", qPrintable(message));
  }
}

void Application::onCommitData(QSessionManager &manager) {
  qDebug("OS asked application to commit its data.");

  manager.setRestartHint(QSessionManager::RestartNever);
  manager.release();
}

void Application::onSaveState(QSessionManager &manager) {
  qDebug("OS asked application to save its state.");

  manager.setRestartHint(QSessionManager::RestartNever);
  manager.release();
}

void Application::onAboutToQuit() {
  eliminateFirstRun();
  eliminateFirstRun(APP_VERSION);

  // Make sure that we obtain close lock BEFORE even trying to quit the application.
  bool locked_safely = feedUpdateLock()->tryLock(4 * CLOSE_LOCK_TIMEOUT);

  processEvents();

  qDebug("Cleaning up resources and saving application state.");

#if defined(Q_OS_WIN)
  system()->removeTrolltechJunkRegistryKeys();
#endif

  mainForm()->tabWidget()->feedMessageViewer()->quit();
  database()->saveDatabase();
  mainForm()->saveSize();
  AdBlockManager::instance()->save();

  if (locked_safely) {
    // Application obtained permission to close in a safe way.
    qDebug("Close lock was obtained safely.");

    // We locked the lock to exit peacefully, unlock it to avoid warnings.
    feedUpdateLock()->unlock();
  }
  else {
    // Request for write lock timed-out. This means
    // that some critical action can be processed right now.
    qDebug("Close lock timed-out.");
  }

  if (m_notification != NULL) {
    m_notification->deleteLater();
    m_notification = NULL;
  }

  // Now, we can check if application should just quit or restart itself.
  if (m_shouldRestart) {
    finish();
    qDebug("Killing local peer connection to allow another instance to start.");

    if (QProcess::startDetached(QString("\"") + QDir::toNativeSeparators(applicationFilePath()) + QString("\""))) {
      qDebug("New application instance was started.");
    }
    else {
      qWarning("New application instance was not started successfully.");
    }
  }
}

void Application::restart() {
  m_shouldRestart = true;
  quit();
}
