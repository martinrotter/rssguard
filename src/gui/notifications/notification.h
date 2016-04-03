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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QObject>

#include <QSystemTrayIcon>


#if defined(Q_OS_LINUX)
class QDBusInterface;
#endif

// Wraps D-Bus notifications.
class Notification : public QObject {
    Q_OBJECT

  public:
    // Constructors.
    explicit Notification(QObject *parent = 0);
    virtual ~Notification();

#if defined(Q_OS_LINUX)
    static bool areDBusNotificationsEnabled();
#endif
    static bool areNotificationsEnabled();

  public slots:
    // Main methods for using the netofication.
    void notify(const QString &text, const QString &title, const QIcon &icon,
                QObject *invokation_target = NULL, const char *invokation_slot = NULL);
    void notify(const QString &text, const QString &title, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                QObject *invokation_target = NULL, const char *invokation_slot = NULL);

    // Cancels display of the notification.
    void cancel();

#if defined(Q_OS_LINUX)
  public slots:
    void notificationClosed(uint id, uint reason);
#endif

  protected:
    void timerEvent(QTimerEvent *event);

  private:
    QString m_title;
    QString m_text;
    QPixmap m_icon;
    int m_timerId;

    QObject *m_clickTarget;
    const char *m_clickSlot;

#if defined(Q_OS_LINUX)
    QDBusInterface *m_dBusInterface;
    uint m_dBusActiveNotification;
#endif
};

#endif // NOTIFICATION_H
