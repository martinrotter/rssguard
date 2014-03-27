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

#include "miscellaneous/settings.h"

#include "definitions/definitions.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QPointer>
#include <QWebSettings>


QPointer<Settings> Settings::s_instance;

Settings::Settings(const QString &file_name, Format format,
                   const Type &status, QObject *parent)
  : QSettings(file_name, format, parent), m_initializationStatus(status) {
}

Settings::~Settings() {
  checkSettings();
  qDebug("Deleting Settings instance.");
}

QSettings::Status Settings::checkSettings() {
  qDebug("Syncing settings.");

  sync();
  return status();
}

Settings *Settings::instance() {
  if (s_instance.isNull()) {
    setupSettings();
  }

  return s_instance;
}

QSettings::Status Settings::setupSettings() {
  // If settings file exists in executable file working directory
  // (in subdirectory APP_CFG_PATH), then use it (portable settings).
  // Otherwise use settings file stored in homePath();
  QString relative_path = QDir::separator() + QString(APP_CFG_PATH) +
                          QDir::separator() + QString(APP_CFG_FILE);

  QString app_path = qApp->applicationDirPath();
  QString app_path_file = app_path + relative_path;

  // Check if portable settings are available.
  if (QFile(app_path_file).exists()) {
    // Portable settings are available, use them.
    s_instance = new Settings(app_path_file, QSettings::IniFormat,
                              Settings::Portable, qApp);

    // Construct icon cache in the same path.
    QString web_path = app_path + QDir::separator() + QString(APP_DB_WEB_PATH);
    QDir(web_path).mkpath(web_path);
    QWebSettings::setIconDatabasePath(web_path);

    qDebug("Initializing settings in '%s' (portable way).",
           qPrintable(QDir::toNativeSeparators(app_path_file)));
  }
  else {
    // Portable settings are NOT available, store them in
    // user's home directory.
    QString home_path = QDir::homePath() + QDir::separator() +
                        QString(APP_LOW_H_NAME);
    QString home_path_file = home_path + relative_path;

    s_instance = new Settings(home_path_file, QSettings::IniFormat,
                              Settings::NonPortable, qApp);

    // Construct icon cache in the same path.
    QString web_path = home_path + QDir::separator() + QString(APP_DB_WEB_PATH);
    QDir(web_path).mkpath(web_path);
    QWebSettings::setIconDatabasePath(web_path);

    qDebug("Initializing settings in '%s' (non-portable way).",
           qPrintable(QDir::toNativeSeparators(home_path_file)));
  }

  return (*s_instance).checkSettings();
}
