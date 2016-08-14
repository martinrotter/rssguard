// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "miscellaneous/feedreader.h"
#include "gui/feedsview.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagebox.h"
#include "gui/statusbar.h"
#include "gui/dialogs/formmain.h"
#include "exceptions/applicationexception.h"

#include "services/abstract/serviceroot.h"
#include "services/standard/standardserviceroot.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"
#include "services/owncloud/owncloudserviceentrypoint.h"

#include <QSessionManager>
#include <QThread>
#include <QProcess>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>


Application::Application(const QString &id, bool run_minimal_without_gui, int &argc, char **argv)
  : QtSingleApplication(id, argc, argv),
    m_runMinimalWithoutGui(run_minimal_without_gui), m_feedReader(new FeedReader(this)),
    m_updateFeedsLock(nullptr), m_userActions(QList<QAction*>()), m_mainForm(nullptr),
    m_trayIcon(nullptr), m_settings(nullptr), m_system(nullptr), m_skins(nullptr),
    m_localization(nullptr), m_icons(nullptr), m_database(nullptr), m_downloadManager(nullptr) {
  connect(this, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
  connect(this, SIGNAL(commitDataRequest(QSessionManager&)), this, SLOT(onCommitData(QSessionManager&)));
  connect(this, SIGNAL(saveStateRequest(QSessionManager&)), this, SLOT(onSaveState(QSessionManager&)));
  connect(QWebEngineProfile::defaultProfile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
          this, SLOT(downloadRequested(QWebEngineDownloadItem*)));
}

Application::~Application() {
  qDebug("Destroying Application instance.");
}

FeedReader *Application::feedReader() {
  return m_feedReader;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != nullptr && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();
  }

  return m_userActions;
}

bool Application::isFirstRun() {
  return settings()->value(GROUP(General), SETTING(General::FirstRun)).toBool();
}

bool Application::isFirstRun(const QString &version) {
  if (version == APP_VERSION) {
    // Check this only if checked version is equal to actual version.
    return settings()->value(GROUP(General), QString(General::FirstRun) + QL1C('_') + version, true).toBool();
  }
  else {
    return false;
  }
}

void Application::eliminateFirstRun() {
  settings()->setValue(GROUP(General), General::FirstRun, false);
}

void Application::eliminateFirstRun(const QString &version) {
  settings()->setValue(GROUP(General), QString(General::FirstRun) + QL1C('_') + version, false);
}

IconFactory *Application::icons() {
  if (m_icons == nullptr) {
    m_icons = new IconFactory(this);
  }

  return m_icons;
}

DownloadManager *Application::downloadManager() {
  if (m_downloadManager == nullptr) {
    m_downloadManager = new DownloadManager();

    connect(m_downloadManager, SIGNAL(downloadFinished()), mainForm()->statusBar(), SLOT(clearProgressDownload()));
    connect(m_downloadManager, SIGNAL(downloadProgress(int,QString)), mainForm()->statusBar(), SLOT(showProgressDownload(int,QString)));
  }

  return m_downloadManager;
}

Mutex *Application::feedUpdateLock() {
  if (m_updateFeedsLock.isNull()) {
    // NOTE: Cannot use parent hierarchy because this method can be usually called
    // from any thread.
    m_updateFeedsLock.reset(new Mutex());
  }

  return m_updateFeedsLock.data();
}

QWidget *Application::mainFormWidget() {
  return m_mainForm;
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

  foreach (const QString &msg, message.split(ARGUMENTS_LIST_SEPARATOR)) {
    if (msg == APP_IS_RUNNING) {
      showGuiMessage(APP_NAME, tr("Application is already running."), QSystemTrayIcon::Information);
      mainForm()->display();
    }
    else if (msg == APP_QUIT_INSTANCE) {
      quit();
    }
    else if (msg.startsWith(QL1S(URI_SCHEME_FEED_SHORT))) {
      // Application was running, and someone wants to add new feed.
      StandardServiceRoot *root = qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->standardServiceRoot();

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

SystemTrayIcon *Application::trayIcon() {
  if (m_trayIcon == nullptr) {
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
  if (m_trayIcon != nullptr) {
    qDebug("Disabling tray icon, deleting it and raising main application window.");
    m_mainForm->display();
    delete m_trayIcon;
    m_trayIcon = nullptr;

    // Make sure that application quits when last window is closed.
    setQuitOnLastWindowClosed(true);
  }
}

void Application::showGuiMessage(const QString &title, const QString &message,
                                 QSystemTrayIcon::MessageIcon message_type, QWidget *parent,
                                 bool show_at_least_msgbox, QObject *invokation_target,
                                 const char *invokation_slot) {
  if (SystemTrayIcon::areNotificationsEnabled() && SystemTrayIcon::isSystemTrayActivated()) {
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
  const bool locked_safely = feedUpdateLock()->tryLock(4 * CLOSE_LOCK_TIMEOUT);

  processEvents();

  qDebug("Cleaning up resources and saving application state.");

#if defined(Q_OS_WIN)
  system()->removeTrolltechJunkRegistryKeys();
#endif

  mainForm()->tabWidget()->feedMessageViewer()->quit();
  database()->saveDatabase();
  mainForm()->saveSize();

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
}

void Application::downloadRequested(QWebEngineDownloadItem *download_item) {
  downloadManager()->download(download_item->url());
  download_item->cancel();
  download_item->deleteLater();
}
