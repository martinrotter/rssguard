// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/systemfactory.h"

#include "exceptions/applicationexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/dialogs/formupdate.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/systemfactory.h"
#include "qtlinq/qtlinq.h"

#include <optional>

#if defined(Q_OS_WIN)
#include <qt_windows.h>

#include <QLibrary>
#include <QOperatingSystemVersion>
#include <QSemaphore>
#include <QSettings>
#endif

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#endif

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVersionNumber>

using UpdateCheck = QPair<UpdateInfo, QNetworkReply::NetworkError>;

#if defined(Q_OS_WIN)
namespace {
  constexpr int WINDOWS_10_MAJOR_VERSION = 10;
  constexpr int WINDOWS_10_1903_BUILD = 18362;
  constexpr int EFFECTIVE_POWER_MODE_CALLBACK_TIMEOUT = 1000;
  constexpr ULONG EFFECTIVE_POWER_MODE_V2_VALUE = 2;
  constexpr int EFFECTIVE_POWER_MODE_GAME_MODE_VALUE = 5;

  using EffectivePowerMode = int;
  using EffectivePowerModeCallback = VOID(CALLBACK*)(EffectivePowerMode mode, PVOID context);
  using PowerRegisterForEffectivePowerModeNotificationsFn = HRESULT(WINAPI*)(ULONG version,
                                                                             EffectivePowerModeCallback callback,
                                                                             PVOID context,
                                                                             PVOID* registration_handle);
  using PowerUnregisterFromEffectivePowerModeNotificationsFn = HRESULT(WINAPI*)(PVOID registration_handle);

  struct EffectivePowerModeState {
      EffectivePowerMode m_mode = -1;
      bool m_received = false;
      QSemaphore m_callbackSemaphore;
  };

  VOID CALLBACK effectivePowerModeCallback(EffectivePowerMode mode, PVOID context) {
    auto* state = static_cast<EffectivePowerModeState*>(context);

    if (state != nullptr) {
      state->m_mode = mode;
      state->m_received = true;
      state->m_callbackSemaphore.release();
    }
  }
} // namespace
#endif

#if defined(Q_OS_LINUX)
namespace {
#define GAME_MODE_DBUS_SERVICE    QSL("com.feralinteractive.GameMode")
#define GAME_MODE_DBUS_PATH       QSL("/com/feralinteractive/GameMode")
#define GAME_MODE_DBUS_INTERFACE  QSL("com.feralinteractive.GameMode")
#define DBUS_PROPERTIES_INTERFACE QSL("org.freedesktop.DBus.Properties")

  bool isGameModeDbusAvailable() {
    auto* dbus_interface = QDBusConnection::sessionBus().interface();

    if (dbus_interface == nullptr) {
      qWarningNN << LOGSEC_CORE << "Game mode: DBus interface is not available.";
      return false;
    }

    if (dbus_interface->isServiceRegistered(GAME_MODE_DBUS_SERVICE)) {
      return true;
    }

    const QDBusReply<QStringList> activatable_services = dbus_interface->activatableServiceNames();

    return activatable_services.isValid() && activatable_services.value().contains(GAME_MODE_DBUS_SERVICE);
  }

  std::optional<int> gameModeClientCount() {
    QDBusInterface properties(GAME_MODE_DBUS_SERVICE,
                              GAME_MODE_DBUS_PATH,
                              DBUS_PROPERTIES_INTERFACE,
                              QDBusConnection::sessionBus());

    if (!properties.isValid()) {
      qDebugNN << LOGSEC_CORE << "Game mode: DBus properties interface is not available.";
      return std::nullopt;
    }

    const QDBusReply<QVariant> reply = properties.call(QSL("Get"), GAME_MODE_DBUS_INTERFACE, QSL("ClientCount"));

    if (!reply.isValid()) {
      qDebugNN << LOGSEC_CORE
               << "Game mode: DBus ClientCount property query failed:" << QUOTE_W_SPACE_DOT(reply.error().message());
      return std::nullopt;
    }

    QVariant value = reply.value();

    if (value.canConvert<QDBusVariant>()) {
      value = qvariant_cast<QDBusVariant>(value).variant();
    }

    bool ok = false;
    const int count = value.toInt(&ok);

    if (!ok) {
      qDebugNN << LOGSEC_CORE << "Game mode: DBus ClientCount property has unexpected value.";
      return std::nullopt;
    }

    return count;
  }

  std::optional<bool> isGameModeActiveViaDbus() {
    const std::optional<int> client_count = gameModeClientCount();

    if (!client_count.has_value()) {
      return std::nullopt;
    }

    qDebugNN << LOGSEC_CORE << "Game mode: DBus client count is" << QUOTE_W_SPACE_DOT(client_count.value());
    return client_count.value() > 0;
  }
} // namespace
#endif

SystemFactory::SystemFactory(QObject* parent) : QObject(parent) {}

SystemFactory::~SystemFactory() = default;

QRegularExpression SystemFactory::supportedUpdateFiles() {
#if defined(Q_OS_WIN)
  return QRegularExpression(QSL(".+win.+\\.(exe|7z)"));
#elif defined(Q_OS_MACOS)
  return QRegularExpression(QSL(".dmg"));
#elif defined(Q_OS_LINUX)
  return QRegularExpression(QSL(".AppImage"));
#else
  return QRegularExpression(QSL(".*"));
#endif
}

SystemFactory::AutoStartStatus SystemFactory::autoStartStatus() {
  // User registry way to auto-start the application on Windows.
#if defined(Q_OS_WIN)
  QSettings registry_key(QSL("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                         QSettings::Format::NativeFormat);
  const bool autostart_enabled = registry_key.contains(QSL(APP_LOW_NAME));

  if (autostart_enabled) {
    return AutoStartStatus::Enabled;
  }
  else {
    return AutoStartStatus::Disabled;
  }
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  // Use proper freedesktop.org way to auto-start the application.
  // INFO: http://standards.freedesktop.org/autostart-spec/latest/
  const QString desktop_file_location = autostartDesktopFileLocation();

  // No correct path was found.
  if (desktop_file_location.isEmpty()) {
    qWarningNN << LOGSEC_GUI << "Searching for auto-start function status failed. HOME variable not found.";
    return AutoStartStatus::Unavailable;
  }

  // We found correct path, now check if file exists and return correct status.
  if (QFile::exists(desktop_file_location)) {
    // File exists, we must read it and check if "Hidden" attribute is defined and what is its value.
    QSettings desktop_settings(desktop_file_location, QSettings::IniFormat);
    bool hidden_value = desktop_settings.value(QSL("Desktop Entry/Hidden"), false).toBool();

    return hidden_value ? AutoStartStatus::Disabled : AutoStartStatus::Enabled;
  }
  else {
    return AutoStartStatus::Disabled;
  }
#else
  // Disable auto-start functionality on unsupported platforms.
  return AutoStartStatus::Unavailable;
#endif
}

bool SystemFactory::isGameModeDetectionSupported() {
#if defined(Q_OS_WIN)
  const QOperatingSystemVersion version = QOperatingSystemVersion::current();

  if (version.type() != QOperatingSystemVersion::Windows) {
    return false;
  }

  return version.majorVersion() > WINDOWS_10_MAJOR_VERSION ||
         (version.majorVersion() == WINDOWS_10_MAJOR_VERSION && version.microVersion() >= WINDOWS_10_1903_BUILD);
#elif defined(Q_OS_LINUX)
  return isGameModeDbusAvailable();
#else
  return false;
#endif
}

bool SystemFactory::isGameModeActive() {
#if defined(Q_OS_WIN)
  if (!isGameModeDetectionSupported()) {
    return false;
  }

  QLibrary power_profile_library(QSL("PowrProf"));

  if (!power_profile_library.load()) {
    qWarningNN << LOGSEC_CORE << "Cannot load PowrProf.dll for Game Mode detection.";
    return false;
  }

  const auto register_notifications = reinterpret_cast<
    PowerRegisterForEffectivePowerModeNotificationsFn>(power_profile_library
                                                         .resolve("PowerRegisterForEffectivePowerModeNotifications"));
  const auto unregister_notifications =
    reinterpret_cast<PowerUnregisterFromEffectivePowerModeNotificationsFn>(power_profile_library
                                                                             .resolve("PowerUnregisterFromEffectivePowe"
                                                                                      "rModeNotifications"));

  if (register_notifications == nullptr || unregister_notifications == nullptr) {
    qWarningNN << LOGSEC_CORE << "Cannot resolve effective power mode functions for Game Mode detection.";
    return false;
  }

  EffectivePowerModeState state;
  PVOID registration_handle = nullptr;
  const HRESULT registration_result =
    register_notifications(EFFECTIVE_POWER_MODE_V2_VALUE, effectivePowerModeCallback, &state, &registration_handle);

  if (registration_result != S_OK) {
    qWarningNN << LOGSEC_CORE << "Cannot register effective power mode callback for Game Mode detection.";
    return false;
  }

  if (registration_handle != nullptr) {
    state.m_received = state.m_callbackSemaphore.tryAcquire(1, EFFECTIVE_POWER_MODE_CALLBACK_TIMEOUT);
    unregister_notifications(registration_handle);
  }

  qDebugNN << LOGSEC_CORE << "Game mode: state received" << QUOTE_W_SPACE(state.m_received) << "with mode"
           << QUOTE_W_SPACE_DOT(state.m_mode);

  return state.m_received && state.m_mode == EFFECTIVE_POWER_MODE_GAME_MODE_VALUE;
#elif defined(Q_OS_LINUX)
  if (!isGameModeDetectionSupported()) {
    return false;
  }

  const std::optional<bool> result = isGameModeActiveViaDbus();

  if (result.has_value()) {
    return result.value();
  }

  return false;
#else
  return false;
#endif
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
QString SystemFactory::autostartDesktopFileLocation() {
  const QString xdg_config_path(qgetenv("XDG_CONFIG_HOME"));
  QString desktop_file_location;

  if (!xdg_config_path.isEmpty()) {
    // XDG_CONFIG_HOME variable is specified. Look for .desktop file
    // in 'autostart' subdirectory.
    desktop_file_location = xdg_config_path + QSL("/autostart/") + APP_REVERSE_NAME + QSL(".desktop");
  }
  else {
    // Desired variable is not set, look for the default 'autostart' subdirectory.
    const QString home_directory(qgetenv("HOME"));

    if (!home_directory.isEmpty()) {
      // Home directory exists. Check if target .desktop file exists and
      // return according status.
      desktop_file_location = home_directory + QSL("/.config/autostart/") + APP_REVERSE_NAME + QSL(".desktop");
    }
  }

  return desktop_file_location;
}

#endif

bool SystemFactory::setAutoStartStatus(AutoStartStatus new_status) {
  const SystemFactory::AutoStartStatus current_status = SystemFactory::autoStartStatus();

  // Auto-start feature is not even available, exit.
  if (current_status == AutoStartStatus::Unavailable) {
    return false;
  }

#if defined(Q_OS_WIN)
  QSettings registry_key(QSL("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                         QSettings::Format::NativeFormat);

  switch (new_status) {
    case AutoStartStatus::Enabled: {
      QStringList args = qlinq::from(qApp->rawCliArgs())
                           .select([](const QString& arg) {
                             if (arg.contains(QL1S(" ")) && !arg.startsWith(QL1S("\""))) {
                               return QSL("\"%1\"").arg(arg);
                             }
                             else {
                               return arg;
                             }
                           })
                           .toList();
      QString app_run_line = args.join(QL1C(' '));
      registry_key.setValue(QSL(APP_LOW_NAME), app_run_line);
      return true;
    }

    case AutoStartStatus::Disabled:
      registry_key.remove(QSL(APP_LOW_NAME));
      return true;

    default:
      return false;
  }
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  // Note that we expect here that no other program uses
  // "rssguard.desktop" desktop file.
  const QString destination_file = autostartDesktopFileLocation();
  const QString destination_folder = QFileInfo(destination_file).absolutePath();

  switch (new_status) {
    case AutoStartStatus::Enabled: {
      if (QFile::exists(destination_file)) {
        if (!QFile::remove(destination_file)) {
          return false;
        }
      }

      if (!QDir().mkpath(destination_folder)) {
        return false;
      }

      const QString source_autostart_desktop_file =
        QString(APP_DESKTOP_ENTRY_PATH) + QDir::separator() + APP_DESKTOP_ENTRY_FILE;

      try {
        QString desktop_file_contents = QString::fromUtf8(IOFactory::readFile(source_autostart_desktop_file));

        QStringList args = qlinq::from(qApp->rawCliArgs())
                             .select([](const QString& arg) {
                               if (arg.contains(QL1S(" ")) && !arg.startsWith(QL1S("\""))) {
                                 return QSL("\"%1\"").arg(arg);
                               }
                               else {
                                 return arg;
                               }
                             })
                             .toList();

#if defined(IS_FLATPAK_BUILD)
        const QString flatpak_run = QSL("flatpak run %1").arg(APP_REVERSE_NAME);

        args = args.mid(1);
        args.prepend(flatpak_run);
#endif

        desktop_file_contents.replace("Exec=@APP_LOW_NAME@", QSL("Exec=") + args.join(QL1C(' ')));
        desktop_file_contents.replace("@APPDATA_SUMMARY@", APPDATA_SUMMARY);
        desktop_file_contents.replace("@APPDATA_NAME@", APPDATA_NAME);
        desktop_file_contents.replace("@APP_REVERSE_NAME@", APP_REVERSE_NAME);
        desktop_file_contents.replace("@APP_LOW_NAME@", APP_LOW_NAME);

        IOFactory::writeFile(destination_file, desktop_file_contents.toUtf8());
      }
      catch (const ApplicationException& ex) {
        return false;
      }

      return true;
    }

    case AutoStartStatus::Disabled:
      return QFile::remove(destination_file);

    default:
      return false;
  }
#else
  return false;
#endif
}

QString SystemFactory::loggedInUser() {
  QString name = qEnvironmentVariable("USER");

  if (name.isEmpty()) {
    name = qEnvironmentVariable("USERNAME");
  }

  if (name.isEmpty()) {
    name = tr("anonymous");
  }

  return name;
}

void SystemFactory::checkForUpdates() const {
  auto* downloader = new Downloader();

  connect(downloader, &Downloader::completed, this, [this, downloader]() {
    QPair<QList<UpdateInfo>, QNetworkReply::NetworkError> result;
    result.second = downloader->lastOutputError();

    if (result.second == QNetworkReply::NoError) {
      QByteArray obtained_data = downloader->lastOutputData();
      result.first = parseUpdatesFile(obtained_data);
    }

    emit updatesChecked(result);
    downloader->deleteLater();
  });
  downloader->downloadFile(QSL(RELEASES_LIST));
}

void SystemFactory::checkForUpdatesOnStartup() {
#if !defined(NO_UPDATE_CHECK)
  if (qApp->settings()->value(GROUP(General), SETTING(General::UpdateOnStartup)).toBool()) {
    QObject::connect(qApp->system(),
                     &SystemFactory::updatesChecked,
                     this,
                     [&](const QPair<QList<UpdateInfo>, QNetworkReply::NetworkError>& updates) {
                       QObject::disconnect(qApp->system(), &SystemFactory::updatesChecked, this, nullptr);

                       if (!updates.first.isEmpty() && updates.second == QNetworkReply::NetworkError::NoError &&
                           SystemFactory::isVersionNewer(updates.first.at(0).m_availableVersion, QSL(APP_VERSION))) {
                         qApp->showGuiMessage(Notification::Event::NewAppVersionAvailable,
                                              {QObject::tr("New version available"),
                                               QObject::tr("Click the bubble for more information."),
                                               QSystemTrayIcon::Information},
                                              {},
                                              {tr("See new version info"), qApp->mainForm(), [] {
                                                 FormUpdate(qApp->mainForm()).exec();
                                               }});
                       }
                     });
    checkForUpdates();
  }
#endif
}

bool SystemFactory::isVersionNewer(const QString& new_version, const QString& base_version) {
  QVersionNumber nw = QVersionNumber::fromString(new_version);
  QVersionNumber bs = QVersionNumber::fromString(base_version);

  return nw > bs;
}

bool SystemFactory::isVersionEqualOrNewer(const QString& new_version, const QString& base_version) {
  return new_version == base_version || isVersionNewer(new_version, base_version);
}

bool SystemFactory::openFolderFile(const QString& file_path) {
#if defined(Q_OS_WIN)
  return QProcess::startDetached(QSL("explorer.exe"), {"/select,", QDir::toNativeSeparators(file_path)});
#else
  const QString folder = QDir::toNativeSeparators(QFileInfo(file_path).absoluteDir().absolutePath());

  return QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
#endif
}

QList<UpdateInfo> SystemFactory::parseUpdatesFile(const QByteArray& updates_file) const {
  QList<UpdateInfo> updates;
  QJsonArray document = QJsonDocument::fromJson(updates_file).array();

  for (QJsonValueRef i : document) {
    QJsonObject release = i.toObject();

    if (release[QSL("tag_name")].toString().contains(QSL("devbuild"))) {
      continue;
    }

    UpdateInfo update;

    update.m_availableVersion = release[QSL("tag_name")].toString();
    update.m_date = QDateTime::fromString(release[QSL("published_at")].toString(), QSL("yyyy-MM-ddTHH:mm:ssZ"));
    update.m_changes = release[QSL("body")].toString();
    QJsonArray assets = release[QSL("assets")].toArray();

    for (QJsonValueRef j : assets) {
      QJsonObject asset = j.toObject();
      UpdateUrl url;

      url.m_fileUrl = asset[QSL("browser_download_url")].toString();
      url.m_name = asset[QSL("name")].toString();
      url.m_size = asset[QSL("size")].toVariant().toString() + tr(" bytes");
      update.m_urls.append(url);
    }

    updates.append(update);
  }

  std::sort(updates.begin(), updates.end(), [](const UpdateInfo& a, const UpdateInfo& b) -> bool {
    return a.m_date > b.m_date;
  });
  return updates;
}
