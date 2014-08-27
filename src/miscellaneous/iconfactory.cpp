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

#include "miscellaneous/iconfactory.h"

#include "miscellaneous/settings.h"

#include <QBuffer>


IconFactory::IconFactory(QObject *parent) : QObject(parent) {
}

IconFactory::~IconFactory() {
  qDebug("Destroying IconFactory instance.");
}

QIcon IconFactory::fromByteArray(QByteArray array) {
  array = QByteArray::fromBase64(array);

  QIcon icon;
  QBuffer buffer(&array);
  buffer.open(QIODevice::ReadOnly);

  QDataStream in(&buffer);
  in >> icon;

  buffer.close();
  return icon;
}

QByteArray IconFactory::toByteArray(const QIcon &icon) {
  QByteArray array;
  QBuffer buffer(&array);
  buffer.open(QIODevice::WriteOnly);

  QDataStream out(&buffer);
  out << icon;

  buffer.close();
  return array.toBase64();
}

void IconFactory::setupSearchPaths() {
  QIcon::setThemeSearchPaths(QStringList() << APP_THEME_PATH);
  qDebug("Available icon theme paths: %s.",
         qPrintable(QIcon::themeSearchPaths()
                    .replaceInStrings(QRegExp("^|$"), "\'")
                    .replaceInStrings(QRegExp("/"), QDir::separator()).join(", ")));
}

void IconFactory::setCurrentIconTheme(const QString &theme_name) {
  qApp->settings()->setValue(APP_CFG_GUI, "icon_theme", theme_name);
}

void IconFactory::loadCurrentIconTheme() {
  QStringList installed_themes = installedIconThemes();
  QString theme_name_from_settings = qApp->settings()->value(APP_CFG_GUI,
                                                             "icon_theme",
                                                             APP_THEME_DEFAULT).toString();

  if (m_currentIconTheme == theme_name_from_settings) {
    qDebug("Icon theme '%s' already loaded.", qPrintable(theme_name_from_settings));
    return;
  }

  // Display list of installed themes.
  qDebug("Installed icon themes are: %s.",
         qPrintable(QStringList(installed_themes).replaceInStrings(QRegExp("^|$"), "\'").join(", ")));

  if (installed_themes.contains(theme_name_from_settings)) {
    // Desired icon theme is installed and can be loaded.
    qDebug("Loading icon theme '%s'.", qPrintable(theme_name_from_settings));
    m_currentIconTheme = theme_name_from_settings;
  }
  else {
    // Desired icon theme is not currently available.
    // Install "default" icon theme instead.
    qDebug("Icon theme '%s' cannot be loaded because it is not installed. No icon theme is loaded now.",
           qPrintable(theme_name_from_settings));
    m_currentIconTheme = APP_NO_THEME;
  }
}

QStringList IconFactory::installedIconThemes() const {
  QStringList icon_theme_names; icon_theme_names << APP_NO_THEME;

  // Iterate all directories with icon themes.
  QStringList icon_themes_paths = QIcon::themeSearchPaths();
  icon_themes_paths.removeDuplicates();

  foreach (const QString &icon_path, icon_themes_paths) {
    QDir icon_dir(icon_path);

    // Iterate all icon themes in this directory.
    foreach (const QString &icon_theme_path, icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                                                QDir::Readable | QDir::CaseSensitive |
                                                                QDir::NoSymLinks,
                                                                QDir::Time)) {
      icon_theme_names << icon_theme_path;
    }
  }

  icon_theme_names.removeDuplicates();
  return icon_theme_names;
}
