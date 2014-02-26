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

#include "gui/systemtrayicon.h"

#include "core/defs.h"
#include "core/settings.h"
#include "gui/formmain.h"
#include "gui/formsettings.h"
#include "qtsingleapplication/qtsingleapplication.h"

#include <QPainter>
#include <QTimer>


#if defined(Q_OS_WIN)
TrayIconMenu::TrayIconMenu(const QString &title, QWidget *parent)
  : QMenu(title, parent) {
}

TrayIconMenu::~TrayIconMenu() {
}

bool TrayIconMenu::event(QEvent *event) {
  if (QtSingleApplication::activeModalWidget() != NULL &&
      event->type() == QEvent::Show) {
    QTimer::singleShot(0, this, SLOT(hide()));
    SystemTrayIcon::instance()->showMessage(APP_LONG_NAME,
                                            tr("Close opened modal dialogs first."),
                                            QSystemTrayIcon::Warning);
  }
  return QMenu::event(event);
}
#endif

QPointer<SystemTrayIcon> SystemTrayIcon::s_trayIcon;

SystemTrayIcon::SystemTrayIcon(const QString &normal_icon,
                               const QString &plain_icon,
                               FormMain *parent)
  : QSystemTrayIcon(parent),
    m_normalIcon(normal_icon),
    m_plainPixmap(plain_icon),
    m_font(QFont())  {
  qDebug("Creating SystemTrayIcon instance.");

  m_font.setBold(true);

  // Initialize icon.
  setNumber();
  setContextMenu(parent->trayMenu());

  // Create necessary connections.
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

SystemTrayIcon::~SystemTrayIcon() {
  qDebug("Destroying SystemTrayIcon instance.");
  hide();
}

void SystemTrayIcon::onActivated(const QSystemTrayIcon::ActivationReason &reason) {
  switch (reason) {
    case SystemTrayIcon::Trigger:
    case SystemTrayIcon::DoubleClick:
    case SystemTrayIcon::MiddleClick:
      static_cast<FormMain*>(parent())->switchVisibility();

    default:
      break;
  }
}

bool SystemTrayIcon::isSystemTrayAvailable() {
  return QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages();
}

bool SystemTrayIcon::isSystemTrayActivated() {
  return SystemTrayIcon::isSystemTrayAvailable() && Settings::instance()->value(APP_CFG_GUI,
                                                                                "use_tray_icon",
                                                                                true).toBool();
}

SystemTrayIcon *SystemTrayIcon::instance() {
  if (s_trayIcon.isNull()) {
    s_trayIcon = new SystemTrayIcon(APP_ICON_PATH,
                                    APP_ICON_PLAIN_PATH,
                                    FormMain::instance());
  }

  return s_trayIcon;
}

void SystemTrayIcon::deleteInstance() {
  if (!s_trayIcon.isNull()) {
    qDebug("Disabling tray icon and raising main application window.");
    static_cast<FormMain*>((*s_trayIcon).parent())->display();
    delete s_trayIcon.data();
    s_trayIcon = NULL;

    // Make sure that application quits when last window is closed.
    qApp->setQuitOnLastWindowClosed(true);
  }
}

void SystemTrayIcon::showPrivate() {
  // Make sure that application does not exit some window (for example
  // the settings window) gets closed. Behavior for main window
  // is handled explicitly by FormMain::closeEvent() method.
  qApp->setQuitOnLastWindowClosed(false);

  // Display the tray icon.
  QSystemTrayIcon::show();
  qDebug("Tray icon displayed.");
}

void SystemTrayIcon::show() {
#if defined(Q_OS_WIN)
  // Show immediately.
  qDebug("Showing tray icon immediately.");
  showPrivate();
#else
  // Delay avoids race conditions and tray icon is properly displayed.
  qDebug("Showing tray icon with 1000 ms delay.");
  QTimer::singleShot(1000,
                     this, SLOT(showPrivate()));
#endif
}

void SystemTrayIcon::setNumber(int number) {
  if (number <= 0) {
    QSystemTrayIcon::setIcon(QIcon(m_normalIcon));
  }
  else {
    QPixmap background(m_plainPixmap);
    QPainter tray_painter;

    tray_painter.begin(&background);
    tray_painter.setBrush(Qt::black);
    tray_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    tray_painter.setRenderHint(QPainter::TextAntialiasing, false);

    // Numbers with more than 2 digits won't be readable, display
    // infinity symbol in that case.
    if (number > 999) {
      m_font.setPixelSize(100);

      tray_painter.setFont(m_font);
      tray_painter.drawText(QRect(0, 0, 128, 128),
                            Qt::AlignVCenter | Qt::AlignCenter ,
                            QChar(8734));
    }
    else {
      // Smaller number if it has 3 digits.
      if (number > 99) {
        m_font.setPixelSize(55);
      }
      else if (number > 9) {
        m_font.setPixelSize(80);
      }
      // Bigger number if it has just one digit.
      else {
        m_font.setPixelSize(100);
      }

      tray_painter.setFont(m_font);
      tray_painter.drawText(QRect(0, 0, 128, 128),
                            Qt::AlignVCenter | Qt::AlignCenter ,
                            QString::number(number));
    }
    tray_painter.end();

    QSystemTrayIcon::setIcon(QIcon(background));
  }
}
