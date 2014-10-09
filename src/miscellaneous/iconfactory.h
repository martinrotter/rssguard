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
    explicit IconFactory(QObject *parent = 0);

    // Destructor.
    virtual ~IconFactory();

    // Used to store/retrieve QIcons from/to database via Base64-encoded
    // byte array.
    QIcon fromByteArray(QByteArray array);
    QByteArray toByteArray(const QIcon &icon);

    // Returns icon from active theme or invalid icon if
    // "no icon theme" is set.
    inline QIcon fromTheme(const QString &name) {
      if (m_currentIconTheme == APP_NO_THEME) {
        return QIcon();
      }

      if (!m_cachedIcons.contains(name)) {
        // Icon is not cached yet.
        m_cachedIcons.insert(name,
                             QIcon(APP_THEME_PATH + QDir::separator() +  m_currentIconTheme + QDir::separator() + name + APP_THEME_SUFFIX));
      }

      return m_cachedIcons.value(name);
    }

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
      return m_currentIconTheme;
    }

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString &theme_name);

    // Singleton getter.
    static IconFactory *instance();

  private:
    QHash<QString, QIcon> m_cachedIcons;
    QString m_currentIconTheme;
};

#endif // ICONFACTORY_H
