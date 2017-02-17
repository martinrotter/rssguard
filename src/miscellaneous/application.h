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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "qtsingleapplication/qtsingleapplication.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iofactory.h"
#include "gui/systemtrayicon.h"
#include "network-web/downloadmanager.h"
#include "core/feeddownloader.h"

#include <QList>

#if defined(qApp)
#undef qApp
#endif

// Define new qApp macro. Yeaaaaah.
#define qApp (Application::instance())


class FormMain;
class IconFactory;
class QAction;
class Mutex;
class QWebEngineDownloadItem;
class FeedReader;

class Application : public QtSingleApplication {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Application(const QString &id, int &argc, char **argv);
    virtual ~Application();

    FeedReader *feedReader();
    void setFeedReader(FeedReader *feed_reader);

    // Globally accessible actions.
    QList<QAction*> userActions();

    // Check whether this application starts for the first time (ever).
    bool isFirstRun();

    // Check whether GIVEN VERSION of the application starts for the first time.
    bool isFirstRun(const QString &version);

    SystemFactory *system();
    SkinFactory *skins();
    Localization *localization();
    DatabaseFactory *database();
    IconFactory *icons();
    DownloadManager *downloadManager();
    Settings *settings();
    Mutex *feedUpdateLock();
    FormMain *mainForm();
    QWidget *mainFormWidget();
    SystemTrayIcon *trayIcon();

    QString getTempFolderPath();
    QString getDocumentsFolderPath();
    QString getHomeFolderPath();

#if defined(Q_OS_LINUX)
    QString getXdgConfigHomePath();
#endif

    void setMainForm(FormMain *main_form);

    void backupDatabaseSettings(bool backup_database, bool backup_settings,
                                const QString &target_path, const QString &backup_name);
    void restoreDatabaseSettings(bool restore_database, bool restore_settings,
                                 const QString &source_database_file_path = QString(),
                                 const QString &source_settings_file_path = QString());

    void showTrayIcon();
    void deleteTrayIcon();

    // Displays given simple message in tray icon bubble or OSD
    // or in message box if tray icon is disabled.
    void showGuiMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon message_type,
                        QWidget *parent = NULL, bool show_at_least_msgbox = false,
                        QObject *invokation_target = NULL, const char *invokation_slot = NULL);

    // Returns pointer to "GOD" application singleton.
    inline static Application *instance() {
      return static_cast<Application*>(QCoreApplication::instance());
    }

  public slots:
    // Processes incoming message from another RSS Guard instance.
    void processExecutionMessage(const QString &message);

  private slots:
    // Last-minute reactors.
    void onCommitData(QSessionManager &manager);
    void onSaveState(QSessionManager &manager);
    void onAboutToQuit();

#if defined(USE_WEBENGINE)
    void downloadRequested(QWebEngineDownloadItem*download_item);
#endif

    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(const Feed *feed, int current, int total);
    void onFeedUpdatesFinished(FeedDownloadResults results);

  private:
    void eliminateFirstRun();
    void eliminateFirstRun(const QString &version);

    FeedReader *m_feedReader;

    // This read-write lock is used by application on its close.
    // Application locks this lock for WRITING.
    // This means that if application locks that lock, then
    // no other transaction-critical action can acquire lock
    // for reading and won't be executed, so no critical action
    // will be running when application quits
    //
    // EACH critical action locks this lock for READING.
    // Several actions can lock this lock for reading.
    // But of user decides to close the application (in other words,
    // tries to lock the lock for writing), then no other
    // action will be allowed to lock for reading.
    QScopedPointer<Mutex> m_updateFeedsLock;

    QList<QAction*> m_userActions;
    FormMain *m_mainForm;
    SystemTrayIcon *m_trayIcon;
    Settings *m_settings;
    SystemFactory *m_system;
    SkinFactory *m_skins;
    Localization *m_localization;
    IconFactory *m_icons;
    DatabaseFactory *m_database;
    DownloadManager *m_downloadManager;
};

#endif // APPLICATION_H
