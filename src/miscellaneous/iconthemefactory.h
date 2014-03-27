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

#ifndef THEMEFACTORY_H
#define THEMEFACTORY_H

#include "definitions/definitions.h"

#include <QString>
#include <QIcon>
#include <QPointer>
#include <QHash>
#include <QApplication>
#include <QDir>


class IconThemeFactory : public QObject {
    Q_OBJECT

  public:
    // Singleton getter.
    static IconThemeFactory *instance();

    // Destructor.
    virtual ~IconThemeFactory();

    // Returns icon from active theme or invalid icon if
    // "no icon theme" is set.
    inline QIcon fromTheme(const QString &name) {
      if (m_currentIconTheme == APP_NO_THEME) {
        return QIcon();
      }

      if (!m_cachedIcons.contains(name)) {
        // Icon is not cached yet.
        m_cachedIcons.insert(name, QIcon(APP_THEME_PATH + QDir::separator() +
                                         m_currentIconTheme + QDir::separator() +
                                         name + APP_THEME_SUFFIX));
      }

      return m_cachedIcons.value(name);
    }

    // Adds custom application path to be search for icons.
    void setupSearchPaths();

    // Returns list of installed themes, including "default" theme.
    QStringList installedIconThemes();

    // Loads name of selected icon theme (from settings) for the application and
    // activates it. If that particular theme is not installed, then
    // "default" theme is loaded.
    void loadCurrentIconTheme();

    // Returns name of currently activated theme for the application.
    QString currentIconTheme();

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString &theme_name);

  private:
    // Constructor.
    explicit IconThemeFactory(QObject *parent = 0);

    QHash<QString, QIcon> m_cachedIcons;
    QString m_currentIconTheme;

    // Singleton.
    static QPointer<IconThemeFactory> s_instance;
};

#endif // THEMEFACTORY_H
