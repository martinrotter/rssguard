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

#include "gui/systemtrayicon.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "gui/dialogs/formmain.h"
#include "gui/dialogs/formsettings.h"

#include <QPainter>
#include <QTimer>


#if defined(Q_OS_WIN)
TrayIconMenu::TrayIconMenu(const QString &title, QWidget *parent) : QMenu(title, parent) {
}

TrayIconMenu::~TrayIconMenu() {
}

bool TrayIconMenu::event(QEvent *event) {
  if (Application::activeModalWidget() != NULL &&
      event->type() == QEvent::Show) {
    QTimer::singleShot(0, this, SLOT(hide()));
    qApp->trayIcon()->showMessage(QSL(APP_LONG_NAME),
                                  tr("Close opened modal dialogs first."),
                                  QSystemTrayIcon::Warning);
  }
  return QMenu::event(event);
}
#endif

SystemTrayIcon::SystemTrayIcon(const QString &normal_icon,
                               const QString &plain_icon,
                               FormMain *parent)
  : QSystemTrayIcon(parent),
    m_normalIcon(normal_icon),
    m_plainPixmap(plain_icon),
    m_font(QFont()),
    m_bubbleClickTarget(NULL),
    m_bubbleClickSlot(NULL) {
  qDebug("Creating SystemTrayIcon instance.");

  m_font.setBold(true);

  // Initialize icon.
  setNumber();
  setContextMenu(parent->trayMenu());

  // Create necessary connections.
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
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
  return SystemTrayIcon::isSystemTrayAvailable() && qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool();
}

void SystemTrayIcon::showPrivate() {
  // Make sure that application does not exit some window (for example
  // the settings window) gets closed. Behavior for main window
  // is handled explicitly by FormMain::closeEvent() method.
  qApp->setQuitOnLastWindowClosed(false);

  // Display the tray icon.
  QSystemTrayIcon::show();
  emit shown();
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
  QTimer::singleShot(1000, this, SLOT(showPrivate()));
#endif
}

void SystemTrayIcon::setNumber(int number, bool any_unread_message) {
  if (number <= 0) {
    setToolTip(QSL(APP_LONG_NAME));
    QSystemTrayIcon::setIcon(QIcon(m_normalIcon));
  }
  else {
    setToolTip(tr("%1\nUnread news: %2").arg(QSL(APP_LONG_NAME), QString::number(number)));

    QPixmap background(m_plainPixmap);
    QPainter tray_painter;

    // TODO: Here draw different background instead of different color of number.
    tray_painter.begin(&background);
    tray_painter.setPen(any_unread_message ? Qt::blue : Qt::black);
    tray_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    tray_painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Numbers with more than 2 digits won't be readable, display
    // infinity symbol in that case.
    if (number > 999) {
      m_font.setPixelSize(100);

      tray_painter.setFont(m_font);
      tray_painter.drawText(QRect(0, 0, 128, 128), Qt::AlignVCenter | Qt::AlignCenter, QChar(8734));
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
      tray_painter.drawText(QRect(0, 0, 128, 128), Qt::AlignVCenter | Qt::AlignCenter, QString::number(number));
    }
    tray_painter.end();

    QSystemTrayIcon::setIcon(QIcon(background));
  }
}

void SystemTrayIcon::showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon,
                                 int milliseconds_timeout_hint, QObject *click_target, const char *click_slot) {
  if (m_bubbleClickTarget != NULL && m_bubbleClickSlot != NULL) {
    // Disconnect previous bubble click signalling.
    disconnect(this, SIGNAL(messageClicked()), m_bubbleClickTarget, m_bubbleClickSlot);
  }

  m_bubbleClickSlot = (char*) click_slot;
  m_bubbleClickTarget = click_target;

  if (click_target != NULL && click_slot != NULL) {
    // Establish new connection for bubble click.
    connect(this, SIGNAL(messageClicked()), click_target, click_slot);
  }

  // NOTE: If connections do not work, then use QMetaObject::invokeMethod(...).
  QSystemTrayIcon::showMessage(title, message, icon, milliseconds_timeout_hint);
}
