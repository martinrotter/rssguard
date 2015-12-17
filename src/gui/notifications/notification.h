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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QWidget>

#include <QSystemTrayIcon>


#if defined(Q_OS_LINUX)
class QDBusInterface;
#endif

class Notification : public QWidget {
    Q_OBJECT

  public:
    // Constructors.
    explicit Notification();
    virtual ~Notification();

    static bool areFancyNotificationsEnabled();
    static bool areNotificationsEnabled();

  public slots:
    // Main methods for using the netofication.
    void notify(const QString &text, const QString &title, const QIcon &icon,
                QObject *invokation_target = NULL, const char *invokation_slot = NULL);
    void notify(const QString &text, const QString &title, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                QObject *invokation_target = NULL, const char *invokation_slot = NULL);

    // Cancels display of the notification.
    void cancel();

    // Loads settings.
    void loadSettings();

#if defined(Q_OS_LINUX)
  private slots:
    void notificationClosed(uint id, uint reason);
#endif

  protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void timerEvent(QTimerEvent *event);

  signals:
    void clicked();

  private:
    void setupWidget();
    void updateGeometries();

    QColor m_backgroundColor;
    QString m_title;
    QString m_text;
    QPixmap m_icon;

    // Defaults to -1, which means "default" (primary) screen.
    int m_screen;
    Qt::Corner m_position;

    // Is calculated according to contents.
    int m_width;
    int m_height;
    int m_padding;
    int m_widgetMargin;
    int m_timerId;

    QObject *m_clickTarget;
    const char *m_clickSlot;

#if defined(Q_OS_LINUX)
    QDBusInterface *m_dBusInterface;
    uint m_dBusActiveNotification;
#endif
};

#endif // NOTIFICATION_H
