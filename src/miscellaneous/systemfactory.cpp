// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "network-web/networkfactory.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "miscellaneous/systemfactory.h"

#if defined(Q_OS_WIN)
#include <QSettings>
#endif

#include <QString>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDomAttr>
#include <QFuture>
#include <QFutureWatcher>

#if QT_VERSION >= 0x050000
#include <QtConcurrent/QtConcurrentRun>
#else
#include <QtConcurrentRun>
#endif


typedef QPair<UpdateInfo, QNetworkReply::NetworkError> UpdateCheck;

SystemFactory::SystemFactory(QObject *parent) : QObject(parent) {
}

SystemFactory::~SystemFactory() {
}

SystemFactory::AutoStartStatus SystemFactory::getAutoStartStatus() {
  // User registry way to auto-start the application on Windows.
#if defined(Q_OS_WIN)
  QSettings registry_key(QSL("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                         QSettings::NativeFormat);
  bool autostart_enabled = registry_key.value(QSL(APP_LOW_NAME),
                                              QString()).toString().replace(QL1C('\\'),
                                                                            QL1C('/')) ==
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
    desktop_file_location = xdg_config_path + QSL("/autostart/") + APP_DESKTOP_ENTRY_FILE;
  }
  else {
    // Desired variable is not set, look for the default 'autostart' subdirectory.
    QString home_directory(qgetenv("HOME"));
    if (!home_directory.isEmpty()) {
      // Home directory exists. Check if target .desktop file exists and
      // return according status.
      desktop_file_location = home_directory + QSL("/.config/autostart/") + APP_DESKTOP_ENTRY_FILE;
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
  QSettings registry_key(QSL("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
  switch (new_status) {
    case SystemFactory::Enabled:
      registry_key.setValue(APP_LOW_NAME,
                            Application::applicationFilePath().replace(QL1C('/'), QL1C('\\')));
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

#if defined(Q_OS_WIN)
bool SystemFactory::removeTrolltechJunkRegistryKeys() {
  if (qApp->settings()->value(GROUP(General), SETTING(General::RemoveTrolltechJunk)).toBool()) {
    QSettings registry_key(QSL("HKEY_CURRENT_USER\\Software\\TrollTech"), QSettings::NativeFormat);

    registry_key.remove(QSL(""));
    registry_key.sync();

    return registry_key.status() == QSettings::NoError;
  }
  else {
    return false;
  }
}
#endif

QString SystemFactory::getUsername() const {
  QString name = qgetenv("USER");

  if (name.isEmpty()) {
    name = qgetenv("USERNAME");
  }

  if (name.isEmpty()) {
    name = tr("anonymous");
  }

  return name;
}

QPair<UpdateInfo, QNetworkReply::NetworkError> SystemFactory::checkForUpdates() {
  QPair<UpdateInfo, QNetworkReply::NetworkError> result;
  QByteArray releases_xml;
  QByteArray changelog;

  result.second = NetworkFactory::downloadFile(RELEASES_LIST, DOWNLOAD_TIMEOUT, releases_xml).first;
  NetworkFactory::downloadFile(CHANGELOG, DOWNLOAD_TIMEOUT, changelog).first;

  if (result.second == QNetworkReply::NoError) {
    result.first = parseUpdatesFile(releases_xml, changelog);
  }

  return result;
}

bool SystemFactory::isUpdateNewer(const QString &update_version) {
  QStringList current_version_tkn = QString(APP_VERSION).split(QL1C('.'));
  QStringList new_version_tkn = update_version.split(QL1C('.'));

  while (!current_version_tkn.isEmpty() && !new_version_tkn.isEmpty()) {
    int current_number = current_version_tkn.takeFirst().toInt();
    int new_number = new_version_tkn.takeFirst().toInt();

    if (new_number > current_number) {
      // New version is indeed higher thatn current version.
      return true;
    }
    else if (new_number < current_number) {
      return false;
    }
  }

  // Versions are either the same or they have unequal sizes.
  if (current_version_tkn.isEmpty() && new_version_tkn.isEmpty()) {
    // Versions are the same.
    return false;
  }
  else {
    if (new_version_tkn.isEmpty()) {
      return false;
    }
    else {
      return new_version_tkn.join(QString()).toInt() > 0;
    }
  }
}

UpdateInfo SystemFactory::parseUpdatesFile(const QByteArray &updates_file, const QByteArray &changelog) {
  UpdateInfo update;
  QDomDocument document; document.setContent(updates_file, false);
  QDomNodeList releases = document.elementsByTagName(QSL("release"));

  if (releases.size() == 1) {
    QDomElement rel_elem = releases.at(0).toElement();

    update.m_availableVersion = rel_elem.attributes().namedItem(QSL("version")).toAttr().value();
    update.m_changes = changelog;

    QDomNodeList urls = rel_elem.elementsByTagName(QSL("url"));

    for (int j = 0; j < urls.size(); j++) {
      UpdateUrl url;
      QDomElement url_elem = urls.at(j).toElement();

      url.m_fileUrl = url_elem.text();
      url.m_os = url_elem.attributes().namedItem(QSL("os")).toAttr().value();
      url.m_platform = url_elem.attributes().namedItem(QSL("platform")).toAttr().value();

      update.m_urls.insert(url.m_os, url);
    }
  }
  else {
    update.m_availableVersion = QString();
  }


  return update;
}

void SystemFactory::checkForUpdatesOnStartup() {
  UpdateCheck updates = checkForUpdates();

  if (updates.second == QNetworkReply::NoError && isUpdateNewer(updates.first.m_availableVersion)) {
    qApp->showGuiMessage(tr("New version available"),
                         tr("Click the bubble for more information."),
                         QSystemTrayIcon::Information,
                         NULL, true, QIcon(), qApp->mainForm(), SLOT(showUpdates()));
  }
}
