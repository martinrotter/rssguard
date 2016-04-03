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

#include "gui/notifications/notification.h"

#include "gui/messagebox.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"

#include <QApplication>
#include <QTimer>

#if defined(Q_OS_LINUX)
#include <QtDBus>
#endif


Notification::Notification(QObject *parent) : QObject(parent), m_title(QString()), m_text(QString()), m_icon(QPixmap()),
  m_timerId(0), m_clickTarget(NULL), m_clickSlot(NULL) {

#if defined(Q_OS_LINUX)
  m_dBusActiveNotification = 0;
  m_dBusInterface = new QDBusInterface("org.freedesktop.Notifications",
                                       "/org/freedesktop/Notifications",
                                       "org.freedesktop.Notifications",
                                       QDBusConnection::sessionBus(), this);

  if (m_dBusInterface->isValid()) {
    QDBusConnection conn = m_dBusInterface->connection();

    if (!conn.connect("org.freedesktop.Notifications",
                      "/org/freedesktop/Notifications",
                      "org.freedesktop.Notifications",
                      "NotificationClosed",
                      this, SLOT(notificationClosed(uint,uint)))) {
      qWarning("Failed to connect notifications to 'NotificationClosed' signal, last error: '%s'.",
               qPrintable(conn.lastError().name()));
    }
  }
#endif
}

Notification::~Notification() {
  qDebug("Destroying Notification instance.");
}

#if defined(Q_OS_LINUX)
bool Notification::areDBusNotificationsEnabled() {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseFancyNotifications)).toBool();
}
#endif

bool Notification::areNotificationsEnabled() {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool();
}

void Notification::notify(const QString &text, const QString &title, const QIcon &icon,
                          QObject *invokation_target, const char *invokation_slot) {
  cancel();

  // Set new values.
  m_clickTarget = invokation_target;
  m_clickSlot = invokation_slot;
  m_text = text;
  m_title = title;
  m_icon = icon.pixmap(NOTIFICATION_ICON_SIZE, NOTIFICATION_ICON_SIZE);

#if defined(Q_OS_LINUX)
  // On Linux, we try to send notification to session notification D-Bus service
  // if it exists.
  if (m_dBusInterface->isValid()) {
    QVariantMap hints;
    hints["image-path"] = ""; // "application-exit";

    QList<QVariant> argument_list;
    argument_list << APP_NAME;      // app_name
    argument_list << (uint)0;       // replace_id
    argument_list << "";            // app_icon
    argument_list << title;         // summary
    argument_list << text;          // body
    argument_list << QStringList(); // actions
    argument_list << hints;         // hints
    argument_list << (int)-1;       // timeout in ms

    QDBusMessage response = m_dBusInterface->callWithArgumentList(QDBus::AutoDetect, "Notify", argument_list);

    if (response.arguments().size() == 1) {
      // Message was sent, notification should display.
      m_dBusActiveNotification = response.arguments().at(0).toUInt();
    }

    return;
  }
#endif

  // Show it.
  m_timerId = startTimer(TRAY_ICON_BUBBLE_TIMEOUT);
}

void Notification::notify(const QString &text, const QString &title, QSystemTrayIcon::MessageIcon icon,
                          QObject *invokation_target, const char *invokation_slot) {
  notify(text, title, MessageBox::iconForStatus((QMessageBox::Icon) icon), invokation_target, invokation_slot);
}

void Notification::cancel() {
  m_clickSlot = NULL;
  m_clickTarget = NULL;

  if (m_timerId != 0) {
    killTimer(m_timerId);
  }
}

void Notification::timerEvent(QTimerEvent *event) {
  QObject::timerEvent(event);
  cancel();
}

#if defined(Q_OS_LINUX)
void Notification::notificationClosed(uint id, uint reason) {
  if (m_clickTarget != NULL && m_clickSlot != NULL && m_dBusActiveNotification == id && reason == 2) {
    QMetaObject::invokeMethod(m_clickTarget, m_clickSlot);
  }
}
#endif
