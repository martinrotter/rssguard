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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "qtsingleapplication/qtsingleapplication.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/systemfactory.h"
#include "gui/systemtrayicon.h"

#include <QMutex>

#if defined(qApp)
#undef qApp
#endif

// Define new qApp macro. Yeaaaaah.
#define qApp (Application::instance())


class FormMain;

// TODO: presunout nektery veci sem, settings atp
class Application : public QtSingleApplication {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Application(const QString &id, int &argc, char **argv);
    virtual ~Application();

    inline SystemFactory *system() {
      if (m_system == NULL) {
        m_system = new SystemFactory(this);
      }

      return m_system;
    }

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
    FormMain *m_mainForm;
    SystemTrayIcon *m_trayIcon;
    Settings *m_settings;
    SystemFactory *m_system;
};

#endif // APPLICATION_H
