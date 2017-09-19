// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FORMUPDATE_H
#define FORMUPDATE_H

#include <QDialog>

#include "ui_formupdate.h"

#include "miscellaneous/systemfactory.h"
#include "network-web/downloader.h"

#include <QNetworkReply>
#include <QPushButton>

class FormUpdate : public QDialog {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit FormUpdate(QWidget* parent);
    virtual ~FormUpdate();

    // Returns true if application can self-update
    // on current platform.
    bool isSelfUpdateSupported() const;

  private slots:

    // Check for updates and interprets the results.
    void checkForUpdates();
    void startUpdate();

    void updateProgress(qint64 bytes_received, qint64 bytes_total);
    void updateCompleted(QNetworkReply::NetworkError status, QByteArray contents);
    void saveUpdateFile(const QByteArray& file_contents);

  private:
    void loadAvailableFiles();

    Ui::FormUpdate m_ui;
    QPushButton* m_btnUpdate;
    Downloader m_downloader;
    QString m_updateFilePath;
    UpdateInfo m_updateInfo;
    bool m_readyToInstall = false;
    qint64 m_lastDownloadedBytes = 0;
};

#endif // FORMUPDATE_H
