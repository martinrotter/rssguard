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

#ifndef SYSTEMFACTORY_H
#define SYSTEMFACTORY_H

#include <QObject>

#include <QPointer>
#include <QMetaType>
#include <QHash>
#include <QPair>
#include <QNetworkReply>


class UpdateUrl {
  public:
    QString m_fileUrl;
    QString m_platform;
    QString m_os;
};

class UpdateInfo {
  public:
    enum UpdateType {
      // Corresponding enum to "maintenace" from UPDATES file.
      Maintenance,
      // Corresponding enum to "evolution" from UPDATES file.
      Evolution
    };

    explicit UpdateInfo() : m_availableVersion(QString()), m_changes(QString()) {
    }

    QString m_availableVersion;
    QString m_changes;
    UpdateType m_type;
    QHash<QString, UpdateUrl> m_urls;
};

Q_DECLARE_METATYPE(UpdateInfo)

class SystemFactory : public QObject {
    Q_OBJECT

  public:
    // Specifies possible states of auto-start functionality.
    enum AutoStartStatus {
      Enabled,
      Disabled,
      Unavailable
    };

    // Constructors and destructors.
    explicit SystemFactory(QObject *parent = 0);

    // Constructors and destructors.
    virtual ~SystemFactory();

    // Returns current status of auto-start function.
    SystemFactory::AutoStartStatus getAutoStartStatus();

    // Sets new status for auto-start function.
    // Function returns false if setting of
    // new status failed.
    bool setAutoStartStatus(const SystemFactory::AutoStartStatus &new_status);

#if defined(Q_OS_WIN)
    bool removeTrolltechJunkRegistryKeys();
#endif

#if defined(Q_OS_LINUX)
    // Returns standard location where auto-start .desktop files
    // should be placed.
    QString getAutostartDesktopFileLocation();
#endif

    // Tries to download list with new updates.
    QPair<UpdateInfo, QNetworkReply::NetworkError> checkForUpdates();

    static bool isUpdateNewer(const QString &update_version);

  public slots:
    // Performs asynchronous check for updates, result is emitted via updateCheckedAsynchronously(...) signal.
    void checkForUpdatesAsynchronously();

  private slots:
    void handleBackgroundUpdatesCheck();

  signals:
    void updateCheckedAsynchronously(QPair<UpdateInfo, QNetworkReply::NetworkError> update_info);

  private:
    // Performs parsing of downloaded file with list of updates.
    UpdateInfo parseUpdatesFile(const QByteArray &updates_file);
};

#endif // SYSTEMFACTORY_H
