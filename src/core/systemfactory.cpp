#include <QString>
#include <QFile>

#if defined(Q_OS_WIN)
#include <QSettings>
#include "qtsingleapplication/qtsingleapplication.h"
#endif

#include "core/systemfactory.h"
#include "core/defs.h"


SystemFactory::SystemFactory() {
}

SystemFactory::AutoStartStatus SystemFactory::getAutoStartStatus() {
  // User registry way to auto-start the application on Windows.
#if defined(Q_OS_WIN)
  QSettings registr_key("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        QSettings::NativeFormat);
  bool autostart_enabled = registr_key.value(APP_LOW_NAME,
                                             "").toString().replace('\\',
                                                                    '/') ==
                           QtSingleApplication::applicationFilePath();

  if (autostart_enabled) {
    return SystemFactory::Enabled;
  }
  else {
    return SystemFactory::Disabled;
  }

  // Use proper freedesktop.org way to auto-start the application on Linux.
  // INFO: http://standards.freedesktop.org/autostart-spec/latest/
#elif defined(Q_OS_LINUX)
  QString xdg_config_path(qgetenv("XDG_CONFIG_HOME"));
  QString desktop_file_location;

  if (!xdg_config_path.isEmpty()) {
    // XDG_CONFIG_HOME variable is specified. Look for .desktop file
    // in 'autostart' subdirectory.
    desktop_file_location = xdg_config_path + "/autostart/rssguard.desktop";
  }
  else {
    // Desired variable is not set, look for the default 'autostart' subdirectory.
    QString home_directory(qgetenv("HOME"));
    if (!home_directory.isEmpty()) {
      // Home directory exists. Check if target .desktop file exists and
      // return according status.
      desktop_file_location = home_directory + "/.config/autostart/rssguard.desktop";
    }
    else {
      qDebug("Searching for auto-start function status failed. HOME variable not found.");
      return SystemFactory::Unavailable;
    }
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

// TODO: Finish implementation of SystemFactory auto-start methods.
bool SystemFactory::setAutoStartStatus(const AutoStartStatus &new_status) {
  SystemFactory::AutoStartStatus current_status = SystemFactory::getAutoStartStatus();

  // Auto-start feature is not even available, exit.
  if (current_status == SystemFactory::Unavailable) {
    return false;
  }

#if defined(Q_OS_WIN)
  QSettings registy_key("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        QSettings::NativeFormat);
  switch (new_status) {
    case SystemFactory::Enabled:
      registy_key.setValue(APP_LOW_NAME,
                           QtSingleApplication::applicationFilePath().replace('/', '\\'));
      return true;
    case SystemFactory::Disabled:
      registy_key.remove(APP_LOW_NAME);
      return true;
    default:
      return false;
  }
#elif defined(Q_OS_LINUX)
  switch (new_status) {
    case SystemFactory::Enabled:

      break;
    case SystemFactory::Disabled:
      break;
    default:
      return false;
  }
#else
  return false;
#endif
}
