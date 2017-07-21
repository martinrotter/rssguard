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

#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include <QObject>

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QString>
#include <QIcon>
#include <QHash>
#include <QDir>


class IconFactory : public QObject {
		Q_OBJECT

	public:
		// Constructor.
		explicit IconFactory(QObject* parent = 0);

		// Destructor.
		virtual ~IconFactory();

		// Used to store/retrieve QIcons from/to Base64-encoded
		// byte array.
		static QIcon fromByteArray(QByteArray array);
		static QByteArray toByteArray(const QIcon& icon);

		QPixmap pixmap(const QString& name);

		// Returns icon from active theme or invalid icon if
		// "no icon theme" is set.
		QIcon fromTheme(const QString& name);

		QPixmap miscPixmap(const QString& name);
		QIcon miscIcon(const QString& name);

		// Adds custom application path to be search for icons.
		void setupSearchPaths();

		// Returns list of installed themes, including "default" theme.
		QStringList installedIconThemes() const;

		// Loads name of selected icon theme (from settings) for the application and
		// activates it. If that particular theme is not installed, then
		// "default" theme is loaded.
		void loadCurrentIconTheme();

		// Returns name of currently activated theme for the application.
		inline QString currentIconTheme() const {
			return QIcon::themeName();
		}

		// Sets icon theme with given name as the active one and loads it.
		void setCurrentIconTheme(const QString& theme_name);
};

#endif // ICONFACTORY_H
