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

#include "miscellaneous/systemfactory.h"

#include "definitions/definitions.h"
#include "network-web/networkfactory.h"
#include "miscellaneous/application.h"

#if defined(Q_OS_WIN)
#include <QSettings>
#endif

#include <QString>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDomAttr>


SystemFactory::SystemFactory(QObject *parent) : QObject(parent) {
}

SystemFactory::~SystemFactory() {
  qDebug("Destroying SystemFactory instance.");
}

SystemFactory::AutoStartStatus SystemFactory::getAutoStartStatus() {
  // User registry way to auto-start the application on Windows.
#if defined(Q_OS_WIN)
  QSettings registry_key("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                         QSettings::NativeFormat);
  bool autostart_enabled = registry_key.value(APP_LOW_NAME,
                                              "").toString().replace('\\',
                                                                     '/') ==
                           Application::applicationFilePath();

  if (autostart_enabled) {
    return SystemFactory::Enabled;
  }
  else {
    return SystemFactory::Disabled;
  }

  // Use proper freedesktop.org way to auto-start the application on Linux.
  // INFO: http://standards.freedesktop.org/autostart-spec/latest/
#elif defined(Q_OS_LINUX)
  QString desktop_file_location = SystemFactory::getAutostartDesktopFileLocation();

  // No correct path was found.
  if (desktop_file_location.isEmpty()) {
    qWarning("Searching for auto-start function status failed. HOME variable not found.");
    return SystemFactory::Unavailable;
  }

  // We found correct path, now check if file exists and return correct status.
  if (QFile::exists(desktop_file_location)) {
    return SystemFactory::Enabled;
  }
  else {
    return SystemFactory::Disabled;
  }

  // Disable auto-start functionality on unsupported platforms.
#else
  return SystemFactory::Unavailable;
#endif
}

#if defined(Q_OS_LINUX)
QString SystemFactory::getAutostartDesktopFileLocation() {
  QString xdg_config_path(qgetenv("XDG_CONFIG_HOME"));
  QString desktop_file_location;

  if (!xdg_config_path.isEmpty()) {
    // XDG_CONFIG_HOME variable is specified. Look for .desktop file
    // in 'autostart' subdirectory.
    desktop_file_location = xdg_config_path + "/autostart/" + APP_DESKTOP_ENTRY_FILE;
  }
  else {
    // Desired variable is not set, look for the default 'autostart' subdirectory.
    QString home_directory(qgetenv("HOME"));
    if (!home_directory.isEmpty()) {
      // Home directory exists. Check if target .desktop file exists and
      // return according status.
      desktop_file_location = home_directory + "/.config/autostart/" + APP_DESKTOP_ENTRY_FILE;
    }
  }

  // No location found, return empty string.
  return desktop_file_location;
}
#endif

bool SystemFactory::setAutoStartStatus(const AutoStartStatus &new_status) {
  SystemFactory::AutoStartStatus current_status = SystemFactory::getAutoStartStatus();

  // Auto-start feature is not even available, exit.
  if (current_status == SystemFactory::Unavailable) {
    return false;
  }

#if defined(Q_OS_WIN)
  QSettings registry_key("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                         QSettings::NativeFormat);
  switch (new_status) {
    case SystemFactory::Enabled:
      registry_key.setValue(APP_LOW_NAME,
                            Application::applicationFilePath().replace('/',
                                                                       '\\'));
      return true;
    case SystemFactory::Disabled:
      registry_key.remove(APP_LOW_NAME);
      return true;
    default:
      return false;
  }
#elif defined(Q_OS_LINUX)
  // Note that we expect here that no other program uses
  // "rssguard.desktop" desktop file.
  switch (new_status) {
    case SystemFactory::Enabled:
      QFile::link(QString(APP_DESKTOP_ENTRY_PATH) + '/' + APP_DESKTOP_ENTRY_FILE,
                  getAutostartDesktopFileLocation());
      return true;
    case SystemFactory::Disabled:
      QFile::remove(getAutostartDesktopFileLocation());
      return true;
    default:
      return false;
  }
#else
  return false;
#endif
}

QPair<UpdateInfo, QNetworkReply::NetworkError> SystemFactory::checkForUpdates() {
  QPair<UpdateInfo, QNetworkReply::NetworkError> result;
  QByteArray releases_xml;

  result.second = NetworkFactory::downloadFile(RELEASES_LIST, DOWNLOAD_TIMEOUT, releases_xml);

  if (result.second == QNetworkReply::NoError) {
    result.first = parseUpdatesFile(releases_xml);
  }

  return result;
}

UpdateInfo SystemFactory::parseUpdatesFile(const QByteArray &updates_file) {
  UpdateInfo update;
  QDomDocument document; document.setContent(updates_file, false);
  QDomNodeList releases = document.elementsByTagName("release");

  if (releases.size() == 1) {
    QDomElement rel_elem = releases.at(0).toElement();
    QString type = rel_elem.attributes().namedItem("type").toAttr().value();

    update.m_availableVersion = rel_elem.attributes().namedItem("version").toAttr().value();
    update.m_changes = rel_elem.namedItem("changes").toElement().text();

    QDomNodeList urls = rel_elem.elementsByTagName("url");

    for (int j = 0; j < urls.size(); j++) {
      UpdateUrl url;
      QDomElement url_elem = urls.at(j).toElement();

      url.m_fileUrl = url_elem.text();
      url.m_os = url_elem.attributes().namedItem("os").toAttr().value();
      url.m_platform = url_elem.attributes().namedItem("platform").toAttr().value();

      update.m_urls.insert(url.m_os,
                           url);
    }

    if (type == "maintenance") {
      update.m_type = UpdateInfo::Maintenance;
    }
    else {
      update.m_type = UpdateInfo::Evolution;
    }
  }
  else {
    update.m_availableVersion = QString();
  }


  return update;
}
