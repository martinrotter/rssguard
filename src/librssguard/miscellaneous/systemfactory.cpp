// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/systemfactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/dialogs/formupdate.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/systemfactory.h"

#if defined(Q_OS_WIN)
#include <QSettings>
#endif

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QString>
#include <QVersionNumber>

using UpdateCheck = QPair<UpdateInfo, QNetworkReply::NetworkError>;

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

SystemFactory::AutoStartStatus SystemFactory::autoStartStatus() const {
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

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
QString SystemFactory::autostartDesktopFileLocation() const {
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
      QStringList args = qApp->rawCliArgs();
      auto std_args = boolinq::from(args.begin(), args.end())
                        .select([](const QString& arg) {
                          if (arg.contains(QL1S(" ")) && !arg.startsWith(QL1S("\""))) {
                            return QSL("\"%1\"").arg(arg);
                          }
                          else {
                            return arg;
                          }
                        })
                        .toStdList();
      args = FROM_STD_LIST(QStringList, std_args);

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

        QStringList args = qApp->rawCliArgs();
        auto std_args = boolinq::from(args.begin(), args.end())
                          .select([](const QString& arg) {
                            if (arg.contains(QL1S(" ")) && !arg.startsWith(QL1S("\""))) {
                              return QSL("\"%1\"").arg(arg);
                            }
                            else {
                              return arg;
                            }
                          })
                          .toStdList();
        args = FROM_STD_LIST(QStringList, std_args);

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

QString SystemFactory::loggedInUser() const {
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
                                              {tr("See new version info"), [] {
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
