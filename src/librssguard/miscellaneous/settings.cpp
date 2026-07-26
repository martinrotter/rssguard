// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/settings.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settingskeys.h"

#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QPointer>

Settings::Settings(const QString& file_name, Format format, SettingsProperties::SettingsType type, QObject* parent)
  : QSettings(file_name, format, parent), m_lock(QReadWriteLock(QReadWriteLock::RecursionMode::Recursive)),
    m_initializationStatus(type) {
  Messages::PreviewerFontStandardDef = QFont(QApplication::font().family(), 10).toString();
}

Settings::~Settings() = default;

QStringList Settings::allKeys(const QString& section) {
  if (!section.isEmpty()) {
    beginGroup(section);
    auto keys = QSettings::allKeys();

    endGroup();
    return keys;
  }
  else {
    return QSettings::allKeys();
  }
}

QString Settings::pathName() const {
  return QFileInfo(fileName()).absolutePath();
}

QSettings::Status Settings::checkSettings() {
  qDebugNN << LOGSEC_CORE << "Syncing settings.";
  sync();
  return status();
}

bool Settings::initiateRestoration(const QString& settings_backup_file_path) {
  return IOFactory::copyFile(settings_backup_file_path,
                             QFileInfo(fileName()).absolutePath() + QDir::separator() + BACKUP_NAME_SETTINGS +
                               BACKUP_SUFFIX_SETTINGS);
}

void Settings::finishRestoration(const QString& desired_settings_file_path) {
  const QString backup_settings_file = QFileInfo(desired_settings_file_path).absolutePath() + QDir::separator() +
                                       BACKUP_NAME_SETTINGS + BACKUP_SUFFIX_SETTINGS;

  if (QFile::exists(backup_settings_file)) {
    qWarningNN << LOGSEC_CORE << "Backup settings file" << QUOTE_W_SPACE(QDir::toNativeSeparators(backup_settings_file))
               << "was detected. Restoring it.";

    if (IOFactory::copyFile(backup_settings_file, desired_settings_file_path)) {
      QFile::remove(backup_settings_file);
      qDebugNN << LOGSEC_CORE << "Settings file was restored successully.";
    }
    else {
      qCriticalNN << LOGSEC_CORE << "Settings file was NOT restored due to error when copying the file.";
    }
  }
}

Settings* Settings::setupSettings(QObject* parent) {
  Settings* new_settings;

  // If settings file exists (and is writable) in executable file working directory
  // (in subdirectory APP_CFG_PATH), then use it (portable settings).
  // Otherwise use settings file stored in home path.
  const SettingsProperties properties = determineProperties();

  finishRestoration(properties.m_absoluteSettingsFileName);

  // Portable settings are available, use them.
  new_settings = new Settings(properties.m_absoluteSettingsFileName, QSettings::IniFormat, properties.m_type, parent);

  if (properties.m_type == SettingsProperties::SettingsType::Portable) {
    qDebugNN << LOGSEC_CORE << "Initializing settings in"
             << QUOTE_W_SPACE(QDir::toNativeSeparators(properties.m_absoluteSettingsFileName)) << "(portable way).";
  }
  else if (properties.m_type == SettingsProperties::SettingsType::Custom) {
    qDebugNN << LOGSEC_CORE << "Initializing settings in"
             << QUOTE_W_SPACE(QDir::toNativeSeparators(properties.m_absoluteSettingsFileName)) << "(custom way).";
  }
  else {
    qDebugNN << LOGSEC_CORE << "Initializing settings in"
             << QUOTE_W_SPACE(QDir::toNativeSeparators(properties.m_absoluteSettingsFileName)) << "(non-portable way).";
  }

  return new_settings;
}

SettingsProperties Settings::determineProperties() {
  SettingsProperties properties;

  properties.m_settingsSuffix = QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE);

  const QString app_path = qApp->userDataAppFolder();
  const QString home_path = qApp->userDataHomeFolder();
  const QString custom_path = qApp->customDataFolder();

  if (!custom_path.isEmpty()) {
    // User wants to have his user data in custom folder, okay.
    properties.m_type = SettingsProperties::SettingsType::Custom;
    properties.m_baseDirectory = custom_path;
  }
  else {
    // We will use PORTABLE settings only if it is available and NON-PORTABLE
    // settings was not initialized before.
#if defined(Q_OS_UNIX) || defined(BUILD_MSYS2)
    // DO NOT use portable settings for *nix or MSYS2, it is really not used on those platforms.
    const bool will_we_use_portable_settings = false;
#else
    const QString exe_path = qApp->applicationDirPath();
    const QString home_path_file = home_path + properties.m_settingsSuffix;
    const bool portable_settings_available = IOFactory::isFolderWritable(exe_path);
    const bool non_portable_settings_exist = QFile::exists(home_path_file);
    const bool will_we_use_portable_settings = portable_settings_available && !non_portable_settings_exist;
#endif

    if (will_we_use_portable_settings) {
      properties.m_type = SettingsProperties::SettingsType::Portable;
      properties.m_baseDirectory = QDir::toNativeSeparators(app_path);
    }
    else {
      properties.m_type = SettingsProperties::SettingsType::NonPortable;
      properties.m_baseDirectory = QDir::toNativeSeparators(home_path);
    }
  }

  properties.m_absoluteSettingsFileName = properties.m_baseDirectory + properties.m_settingsSuffix;
  return properties;
}
