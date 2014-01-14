#include "core/systemfactory.h"

#include "core/defs.h"

#if defined(Q_OS_WIN)
#include "qtsingleapplication/qtsingleapplication.h"

#include <QSettings>
#endif

#include <QString>
#include <QFile>
#include <QApplication>
#include <QReadWriteLock>


QPointer<SystemFactory> SystemFactory::s_instance;

SystemFactory::SystemFactory(QObject *parent) : QObject(parent) {
  m_applicationCloseLock = new QReadWriteLock(QReadWriteLock::NonRecursive);
}

SystemFactory::~SystemFactory() {
  qDebug("Destroying SystemFactory instance.");

  delete m_applicationCloseLock;
}


SystemFactory::AutoStartStatus SystemFactory::getAutoStartStatus() {
  // User registry way to auto-start the application on Windows.
#if defined(Q_OS_WIN)
  QSettings registry_key("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        QSettings::NativeFormat);
  bool autostart_enabled = registry_key.value(APP_LOW_NAME,
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

SystemFactory *SystemFactory::getInstance() {
  if (s_instance.isNull()) {
    s_instance = new SystemFactory(qApp);
  }

  return s_instance;
}



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
                            QtSingleApplication::applicationFilePath().replace('/',
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
