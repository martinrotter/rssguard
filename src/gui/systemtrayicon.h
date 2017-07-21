// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "definitions/definitions.h"

#include <QPixmap>
#include <QMenu>


class FormMain;
class QEvent;

#if defined(Q_OS_WIN)
class TrayIconMenu : public QMenu {
		Q_OBJECT

	public:
		// Constructors and destructors.
		explicit TrayIconMenu(const QString& title, QWidget* parent);
		virtual ~TrayIconMenu();

	protected:
		bool event(QEvent* event);
};
#endif

class SystemTrayIcon : public QSystemTrayIcon {
		Q_OBJECT

	public:
		// Constructors and destructors.
		explicit SystemTrayIcon(const QString& normal_icon,
		                        const QString& plain_icon,
		                        FormMain* parent = 0);
		virtual ~SystemTrayIcon();

		// Sets the number to be visible in the tray icon, number <= 0 removes it.
		void setNumber(int number = -1, bool any_new_message = false);

		void showMessage(const QString& title, const QString& message, MessageIcon icon = Information,
		                 int milliseconds_timeout_hint = TRAY_ICON_BUBBLE_TIMEOUT, QObject* click_target = nullptr,
		                 const char* click_slot = nullptr);

		// Returns true if tray icon CAN be constructed on this machine.
		static bool isSystemTrayAvailable();

		// Returns true if tray icon CAN be costructed and IS enabled in
		// application settings.
		static bool isSystemTrayActivated();

		// Determines whether balloon tips are enabled or not on tray icons.
		static bool areNotificationsEnabled();

	public slots:
		void show();

	private slots:
		void showPrivate();
		void onActivated(const QSystemTrayIcon::ActivationReason& reason);

	signals:
		void shown();

	private:
		QIcon m_normalIcon;
		QPixmap m_plainPixmap;
		QFont m_font;

		QObject* m_bubbleClickTarget;
		char* m_bubbleClickSlot;
};

#endif // SYSTEMTRAYICON_H
