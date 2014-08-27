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

#include "miscellaneous/application.h"

#include "miscellaneous/iconfactory.h"
#include "gui/feedsview.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagebox.h"
#include "gui/formmain.h"

#include <QThread>


Application::Application(const QString &id, int &argc, char **argv)
  : QtSingleApplication(id, argc, argv),
    m_closeLock(NULL), m_userActions(QList<QAction*>()), m_mainForm(NULL),
    m_trayIcon(NULL), m_settings(NULL), m_system(NULL), m_skins(NULL),
    m_localization(NULL), m_icons(NULL), m_database(NULL) {
}

Application::~Application() {
  delete m_closeLock;
}

QList<QAction*> Application::userActions() {
  if (m_mainForm != NULL && m_userActions.isEmpty()) {
    m_userActions = m_mainForm->allActions();
  }

  return m_userActions;
}

IconFactory *Application::icons() {
  if (m_icons == NULL) {
    m_icons = new IconFactory(this);
  }

  return m_icons;
}

SystemTrayIcon *Application::trayIcon() {
  if (m_trayIcon == NULL) {
    m_trayIcon = new SystemTrayIcon(APP_ICON_PATH, APP_ICON_PLAIN_PATH, m_mainForm);
    m_trayIcon->setToolTip(APP_LONG_NAME);
  }

  return m_trayIcon;
}

void Application::showTrayIcon() {
  qDebug("Showing tray icon.");
  trayIcon()->show();

  if (m_mainForm != NULL) {
    m_mainForm->tabWidget()->feedMessageViewer()->feedsView()->notifyWithCounts();
  }
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

void Application::showGuiMessage(const QString& title, const QString& message,
                                 QSystemTrayIcon::MessageIcon message_type,
                                 QWidget *parent, int duration) {
  if (SystemTrayIcon::isSystemTrayActivated()) {
    // TODO: Maybe show OSD instead if tray icon bubble,
    // depending on settings.
    trayIcon()->showMessage(title, message, message_type, duration);
  }
  else {
    // TODO: Tray icon or OSD is not available, display simple text box.
    MessageBox::show(parent, (QMessageBox::Icon) message_type, title, message);
  }
}
