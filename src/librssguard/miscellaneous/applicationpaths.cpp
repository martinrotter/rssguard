// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/applicationpaths.h"

#include "definitions/globals.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settings.h"

#include <QDir>
#include <QStandardPaths>
#include <QVersionNumber>

ApplicationPaths::ApplicationPaths(Application* application) : m_application(application) {}

QString ApplicationPaths::tempFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation);
}

QString ApplicationPaths::documentsFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::DocumentsLocation);
}

QString ApplicationPaths::homeFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::HomeLocation);
}

QString ApplicationPaths::configFolder() const {
  return IOFactory::getSystemFolder(QStandardPaths::StandardLocation::GenericConfigLocation);
}

QString ApplicationPaths::userDataAppFolder() const {
  static const int major_version = QVersionNumber::fromString(QSL(APP_VERSION)).majorVersion();

  return QDir::toNativeSeparators(m_application->applicationDirPath() + QDir::separator() +
                                  QSL("data%1").arg(major_version));
}

QString ApplicationPaths::userDataHomeFolder() const {
  static const int major_version = QVersionNumber::fromString(QSL(APP_VERSION)).majorVersion();

  return configFolder() + QDir::separator() + QSL(APP_LOW_NAME) + QString::number(major_version);
}

QString ApplicationPaths::customDataFolder() const {
  return QDir::toNativeSeparators(m_customDataFolder);
}

QString ApplicationPaths::userDataFolder() const {
  if (m_application->settings()->type() == SettingsProperties::SettingsType::Custom) {
    return customDataFolder();
  }
  else if (m_application->settings()->type() == SettingsProperties::SettingsType::Portable) {
    return userDataAppFolder();
  }

  return userDataHomeFolder();
}

QString ApplicationPaths::replaceUserDataFolderPlaceholder(QString text, bool double_escape) const {
  auto user_data_folder = userDataFolder();

  return text.replace(QSL(USER_DATA_PLACEHOLDER),
                      double_escape ? user_data_folder.replace(QDir::separator(), QString(2, QDir::separator()))
                                    : user_data_folder);
}

QStringList ApplicationPaths::replaceUserDataFolderPlaceholder(QStringList texts) const {
  return texts.replaceInStrings(QSL(USER_DATA_PLACEHOLDER), userDataFolder());
}

bool ApplicationPaths::setCustomDataFolder(const QString& data_folder) {
  if (!QDir().mkpath(data_folder)) {
    qCriticalNN << LOGSEC_CORE << "Failed to create custom data path" << QUOTE_W_SPACE(data_folder)
                << "thus falling back to standard setup.";
    m_customDataFolder.clear();
    return false;
  }

  m_customDataFolder = data_folder;
  return true;
}
