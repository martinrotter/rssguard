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

#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include <QSystemTrayIcon>
#include <QPointer>
#include <QPixmap>
#include <QMenu>


class FormMain;
class QEvent;

#if defined(Q_OS_WIN)
class TrayIconMenu : public QMenu {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit TrayIconMenu(const QString &title, QWidget *parent);
    virtual ~TrayIconMenu();

  protected:
    bool event(QEvent *event);
};
#endif

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit SystemTrayIcon(const QString &normal_icon,
                            const QString &plain_icon,
                            FormMain *parent = 0);
    virtual ~SystemTrayIcon();

    // Returns true if tray icon CAN be constructed on this machine.
    static bool isSystemTrayAvailable();

    // Returns true if tray icon CAN be costructed and IS enabled in
    // application settings.
    static bool isSystemTrayActivated();

    // Creates new tray icon if necessary and returns it.
    // WARNING: Use this in cooperation with SystemTrayIcon::isSystemTrayActivated().
    static SystemTrayIcon *instance();
    
    // Sets the number to be visible in the tray icon, number <= 0 removes it.
    void setNumber(int number = -1);

    // Explicitle clears SystemTrayIcon instance from the memory.
    static void deleteInstance();
    
  public slots:
    void show();

  private slots:
    void showPrivate();
    void onActivated(const QSystemTrayIcon::ActivationReason &reason);

  private:
    QIcon m_normalIcon;
    QPixmap m_plainPixmap;
    QFont m_font;

    static QPointer<SystemTrayIcon> s_trayIcon;
};

#endif // SYSTEMTRAYICON_H
