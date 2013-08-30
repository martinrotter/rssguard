/*
    This file is part of Qonverter.

    Qonverter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Qonverter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Qonverter.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2012 - 2013 Martin Rotter
*/

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QPointer>

#include "core/settings.h"
#include "core/defs.h"


QPointer<Settings> Settings::s_instance;

Settings::Settings(const QString &file_name, Format format, QObject *parent)
  : QSettings(file_name, format, parent) {
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

Settings *Settings::getInstance() {
  if (s_instance.isNull()) {
    setupSettings();
  }

  return s_instance;
}

QVariant Settings::value(const QString &section,
                         const QString &key,
                         const QVariant &default_value) {
  return QSettings::value(QString("%1/%2").arg(section, key), default_value);
}

void Settings::setValue(const QString &section,
                        const QString &key,
                        const QVariant &value) {
  QSettings::setValue(QString("%1/%2").arg(section, key), value);
}

QSettings::Status Settings::setupSettings() {
  // If settings file exists in executable file working directory,
  // then use it (portable settings).
  // Otherwise use settings file stored in homePath();
  QString home_path = QDir::homePath() + QDir::separator() +
                      APP_LOW_H_NAME + QDir::separator() +
                      APP_CFG_PATH;
  QString app_path = qApp->applicationDirPath() + QDir::separator() +
                     APP_CFG_PATH;

  if (QFile(app_path).exists()) {
    s_instance = new Settings(app_path, QSettings::IniFormat, qApp);
    qDebug("Initializing settings in %s.",
           qPrintable(QDir::toNativeSeparators(app_path)));
  }
  else {
    s_instance = new Settings(home_path, QSettings::IniFormat, qApp);
    qDebug("Initializing settings in %s.",
           qPrintable(QDir::toNativeSeparators(home_path)));
  }

  return (*s_instance).checkSettings();
}
