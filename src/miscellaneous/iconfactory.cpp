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
  in.setVersion(QDataStream::Qt_4_7);
  in >> icon;

  buffer.close();
  return icon;
}

QByteArray IconFactory::toByteArray(const QIcon &icon) {
  QByteArray array;
  QBuffer buffer(&array);
  buffer.open(QIODevice::WriteOnly);

  QDataStream out(&buffer);
  out.setVersion(QDataStream::Qt_4_7);
  out << icon;

  buffer.close();
  return array.toBase64();
}

QPixmap IconFactory::pixmap(const QString &name) {
  if (QIcon::themeName() == APP_NO_THEME) {
    return QPixmap();
  }
  else {
    return QIcon::fromTheme(name).pixmap(64, 64);
  }
}

QIcon IconFactory::fromTheme(const QString &name) {
  return QIcon::fromTheme(name);
}

QPixmap IconFactory::miscPixmap(const QString &name) {
  return QPixmap(QString(APP_THEME_PATH) + QDir::separator() + "misc" + QDir::separator() + name + ".png");
}

QIcon IconFactory::miscIcon(const QString &name) {
  return QIcon(QString(APP_THEME_PATH) + QDir::separator() + "misc" + QDir::separator() + name + ".png");
}

void IconFactory::setupSearchPaths() {
  QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << APP_THEME_PATH);
  qDebug("Available icon theme paths: %s.",
         qPrintable(QIcon::themeSearchPaths()
                    .replaceInStrings(QRegExp(QSL("^|$")), QSL("\'"))
                    .replaceInStrings(QRegExp(QSL("/")), QDir::separator()).join(QSL(", "))));
}

void IconFactory::setCurrentIconTheme(const QString &theme_name) {
  qApp->settings()->setValue(GROUP(GUI), GUI::IconTheme, theme_name);
}

void IconFactory::loadCurrentIconTheme() {
  const QStringList installed_themes = installedIconThemes();
  const QString theme_name_from_settings = qApp->settings()->value(GROUP(GUI), SETTING(GUI::IconTheme)).toString();

  if (QIcon::themeName() == theme_name_from_settings) {
    qDebug("Icon theme '%s' already loaded.", qPrintable(theme_name_from_settings));
    return;
  }

  // Display list of installed themes.
  qDebug("Installed icon themes are: %s.",
         qPrintable(QStringList(installed_themes).replaceInStrings(QRegExp(QSL("^|$")), QSL("\'")).join(QSL(", "))));

  if (installed_themes.contains(theme_name_from_settings)) {
    // Desired icon theme is installed and can be loaded.
    qDebug("Loading icon theme '%s'.", qPrintable(theme_name_from_settings));
    QIcon::setThemeName(theme_name_from_settings);
  }
  else {
    // Desired icon theme is not currently available.
    // Install "default" icon theme instead.
    qDebug("Icon theme '%s' cannot be loaded because it is not installed. No icon theme is loaded now.",
           qPrintable(theme_name_from_settings));
    QIcon::setThemeName(APP_NO_THEME);
  }
}

QStringList IconFactory::installedIconThemes() const {
  QStringList icon_theme_names; icon_theme_names << APP_NO_THEME;

  // Iterate all directories with icon themes.
  QStringList icon_themes_paths = QIcon::themeSearchPaths();
  QStringList filters_index;

  filters_index.append("index.theme");
  icon_themes_paths.removeDuplicates();

  foreach (const QString &icon_path, icon_themes_paths) {
    const QDir icon_dir(icon_path);

    // Iterate all icon themes in this directory.
    foreach (const QFileInfo &icon_theme_path, icon_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot |
                                                                      QDir::Readable | QDir::CaseSensitive |
                                                                      QDir::NoSymLinks,
                                                                      QDir::Time)) {
      QDir icon_theme_dir = QDir(icon_theme_path.absoluteFilePath());

      if (icon_theme_dir.exists(filters_index.at(0))) {
        // We found theme.
        QSettings index_theme(icon_theme_dir.entryInfoList(filters_index,
                                                           QDir::Files).at(0).absoluteFilePath(), QSettings::IniFormat);

        icon_theme_names << index_theme.value(QString("%1/%2").arg("Icon Theme",
                                                                   "Name")).toString();
      }
    }
  }

  icon_theme_names.removeDuplicates();
  icon_theme_names.removeAll(QString());

  return icon_theme_names;
}
