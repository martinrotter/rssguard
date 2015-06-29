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


class Notification : public QWidget {
    Q_OBJECT

  public:
    // Constructors.
    explicit Notification();
    virtual ~Notification();

    static bool areNotificationsActivated();

  public slots:
    void notify(const QString &text, const QString &title, const QIcon &icon);
    void notify(const QString &text, const QString &title, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);

  protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

  private:
    void loadSettings();
    void setupWidget();
    void updateGeometries();

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
};

#endif // NOTIFICATION_H
