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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "qtsingleapplication/qtsingleapplication.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/databasefactory.h"
#include "gui/systemtrayicon.h"

#include <QMutex>
#include <QList>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

#if defined(qApp)
#undef qApp
#endif

// Define new qApp macro. Yeaaaaah.
#define qApp (Application::instance())


class FormMain;
class IconFactory;
class QAction;

class Application : public QtSingleApplication {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Application(const QString &id, int &argc, char **argv);
    virtual ~Application();

    QList<QAction*> userActions();

    inline SystemFactory *system() {
      if (m_system == NULL) {
        m_system = new SystemFactory(this);
      }

      return m_system;
    }

    inline SkinFactory *skins() {
      if (m_skins == NULL) {
        m_skins = new SkinFactory(this);
      }

      return m_skins;
    }

    inline Localization *localization() {
      if (m_localization == NULL) {
        m_localization = new Localization(this);
      }

      return m_localization;
    }

    inline DatabaseFactory *database() {
      if (m_database == NULL) {
        m_database = new DatabaseFactory(this);
      }

      return m_database;
    }

    IconFactory *icons();

    inline Settings *settings() {
      if (m_settings == NULL) {
        m_settings = Settings::setupSettings(this);
      }

      return m_settings;
    }

    // Access to application-wide close lock.
    inline QMutex *closeLock() {
      if (m_closeLock == NULL) {
        m_closeLock = new QMutex();
      }

      return m_closeLock;
    }

    inline FormMain *mainForm() {
      return m_mainForm;
    }

    void setMainForm(FormMain *main_form) {
      m_mainForm = main_form;
    }

    inline QString tempFolderPath() {
#if QT_VERSION >= 0x050000
      QString temp_directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
      QString temp_directory = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif
      return temp_directory;
    }

    inline QString documentsFolderPath() {
#if QT_VERSION >= 0x050000
      QString doc_directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
      QString doc_directory = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
      return doc_directory;
    }

    inline QString homeFolderPath() {
#if QT_VERSION >= 0x050000
      QString home_path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#else
      QString home_path = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
#endif
      return home_path;
    }

    bool backupDatabaseSettings(bool backup_database, bool backup_settings, const QString &target_path, const QString &backup_name);
    bool restoreDatabaseSettings(bool restore_database, bool restore_settings,
                                 const QString &source_database_file_path = QString(),
                                 const QString &source_settings_file_path = QString());

    // Access to application tray icon. Always use this in cooperation with
    // SystemTrayIcon::isSystemTrayActivated().
    SystemTrayIcon *trayIcon();

    void showTrayIcon();
    void deleteTrayIcon();

    // Displays given simple message in tray icon bubble or OSD
    // or in message box if tray icon is disabled.
    void showGuiMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon message_type,
                        QWidget *parent = NULL, int duration = TRAY_ICON_BUBBLE_TIMEOUT);

    // Returns pointer to "GOD" application singleton.
    inline static Application *instance() {
      return static_cast<Application*>(QCoreApplication::instance());
    }

    bool shouldRestart() const;
    void setShouldRestart(bool shouldRestart);

  public slots:
    void restart();

    // Processes incoming message from another RSS Guard instance.
    void processExecutionMessage(const QString &message);

  private slots:
    // Last-minute reactors.
    void onCommitData(QSessionManager &manager);
    void onSaveState(QSessionManager &manager);
    void onAboutToQuit();

  private:
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
    QMutex *m_closeLock;
    QList<QAction*> m_userActions;
    FormMain *m_mainForm;
    SystemTrayIcon *m_trayIcon;
    Settings *m_settings;
    SystemFactory *m_system;
    SkinFactory *m_skins;
    Localization *m_localization;
    IconFactory *m_icons;
    DatabaseFactory *m_database;
    bool m_shouldRestart;
};

#endif // APPLICATION_H
