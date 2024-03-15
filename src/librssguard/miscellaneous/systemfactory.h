// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SYSTEMFACTORY_H
#define SYSTEMFACTORY_H

#include <QObject>

#include "network-web/downloader.h"

#include <QHash>
#include <QMetaType>
#include <QNetworkReply>
#include <QPair>
#include <QRegularExpression>

class RSSGUARD_DLLSPEC UpdateUrl {
  public:
    QString m_fileUrl;
    QString m_name;
    QString m_size;
};

class RSSGUARD_DLLSPEC UpdateInfo {
  public:
    QString m_availableVersion;
    QString m_changes;
    QDateTime m_date;
    QList<UpdateUrl> m_urls;
};

Q_DECLARE_METATYPE(UpdateInfo)

class RSSGUARD_DLLSPEC SystemFactory : public QObject {
    Q_OBJECT

  public:
    // Specifies possible states of auto-start functionality.
    enum class AutoStartStatus {
      Enabled,
      Disabled,
      Unavailable
    };

    explicit SystemFactory(QObject* parent = nullptr);
    virtual ~SystemFactory();

    // Returns current status of auto-start function.
    SystemFactory::AutoStartStatus autoStartStatus() const;

    // Sets new status for auto-start function.
    // Function returns false if setting of
    // new status failed.
    bool setAutoStartStatus(AutoStartStatus new_status);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    // Returns standard location where auto-start .desktop files
    // should be placed.
    QString autostartDesktopFileLocation() const;
#endif

    // Retrieves username of currently logged-in user.
    QString loggedInUser() const;

    // Tries to download list with new updates.
    void checkForUpdates() const;

  public slots:
    void checkForUpdatesOnStartup();

    static QRegularExpression supportedUpdateFiles();

    // Checks if update is newer than current application version.
    static bool isVersionNewer(const QString& new_version, const QString& base_version);
    static bool isVersionEqualOrNewer(const QString& new_version, const QString& base_version);
    static bool openFolderFile(const QString& file_path);

  signals:
    void updatesChecked(QPair<QList<UpdateInfo>, QNetworkReply::NetworkError> updates) const;

  private:
    // Performs parsing of downloaded file with list of updates.
    QList<UpdateInfo> parseUpdatesFile(const QByteArray& updates_file) const;
};

#endif // SYSTEMFACTORY_H
