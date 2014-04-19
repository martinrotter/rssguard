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

#include "gui/formupdate.h"

#include "definitions/definitions.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "network-web/downloader.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"

#include <QNetworkReply>
#include <QDesktopServices>
#include <QProcess>


FormUpdate::FormUpdate(QWidget *parent)
  : QDialog(parent), m_downloader(NULL), m_readyToInstall(false), m_ui(new Ui::FormUpdate) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconFactory::instance()->fromTheme("application-about"));

  m_btnUpdate = m_ui->m_buttonBox->addButton(tr("Update"), QDialogButtonBox::ActionRole);
  m_btnUpdate->setToolTip(tr("Download new installation files."));

  connect(m_btnUpdate, SIGNAL(clicked()), this, SLOT(startUpdate()));

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  m_ui->m_lblCurrentRelease->setText(APP_VERSION);
  checkForUpdates();
}

FormUpdate::~FormUpdate() {
  delete m_ui;
}

bool FormUpdate::isUpdateForThisSystem() const {
  return m_updateInfo.m_urls.keys().contains(OS_ID);
}

bool FormUpdate::isSelfUpdateSupported() const {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
  return true;
#else
  return false;
#endif
}

void FormUpdate::checkForUpdates() {
  QPair<UpdateInfo, QNetworkReply::NetworkError> update = SystemFactory::instance()->checkForUpdates();

  m_updateInfo = update.first;

  if (update.second != QNetworkReply::NoError) {
    //: Unknown release.
    m_ui->m_lblAvailableRelease->setText(tr("unknown"));
    m_ui->m_txtChanges->clear();
    m_ui->m_lblStatus->setStatus(WidgetWithStatus::Error,
                                 tr("Error: '%1'.").arg(NetworkFactory::networkErrorText(update.second)),
                                 tr("List with updates was "
                                    "not\ndownloaded successfully."));
    m_btnUpdate->setEnabled(false);
    m_btnUpdate->setToolTip(tr("Checking for updates failed."));
  }
  else {
    m_ui->m_lblAvailableRelease->setText(update.first.m_availableVersion);
    m_ui->m_txtChanges->setText(update.first.m_changes);

    if (update.first.m_availableVersion > APP_VERSION) {
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
                                   tr("New release available."),
                                   tr("This is new version which can be\ndownloaded and installed."));
      m_btnUpdate->setEnabled(true);
      m_btnUpdate->setToolTip(isUpdateForThisSystem() ?
                                tr("Download installation file for your OS.") :
                                tr("Installation file is not available directly.\n"
                                   "Go to application website to obtain it manually."));

      if (isUpdateForThisSystem() && isSelfUpdateSupported()) {
        m_btnUpdate->setText(tr("Download update"));
      }
    }
    else {
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Warning,
                                   tr("No new release available."),
                                   tr("This release is not newer than\ncurrently installed one."));
      m_btnUpdate->setEnabled(false);
      m_btnUpdate->setToolTip(tr("No new update available."));
    }
  }
}

void FormUpdate::updateProgress(qint64 bytes_received, qint64 bytes_total) {
  qApp->processEvents();
  m_ui->m_lblStatus->setStatus(WidgetWithStatus::Information,
                               tr("Downloaded %1% (update size is %2 kB).").arg(QString::number((bytes_received * 100.0) / bytes_total,
                                                                                                'f',
                                                                                                2),
                                                                                QString::number(bytes_total / 1000,
                                                                                                'f',
                                                                                                2)),
                               tr("Downloading update..."));
}

void FormUpdate::saveUpdateFile(const QByteArray &file_contents) {
  QString url_file = m_updateInfo.m_urls.value(OS_ID).m_fileUrl;;

#if QT_VERSION >= 0x050000
  QString temp_directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
  QString temp_directory = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

  if (!temp_directory.isEmpty()) {
    QString output_file_name = url_file.mid(url_file.lastIndexOf('/') + 1);
    QFile output_file(temp_directory + QDir::separator() + output_file_name);

    if (output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      qDebug("Storing update file to temporary location '%s'.",
             qPrintable(output_file_name));

      output_file.write(file_contents);
      output_file.flush();
      output_file.close();

      qDebug("Update file contents was successfuly saved.");

      m_updateFilePath = output_file.fileName();
      m_readyToInstall = true;
    }
    else {
      qDebug("Cannot save downloaded update file because target temporary file '%s' cannot be "
             "opened for writing.", qPrintable(output_file_name));
    }
  }
  else {
    qDebug("Cannot save downloaded update file because no TEMP folder is available.");
  }
}

void FormUpdate::updateCompleted(QNetworkReply::NetworkError status, QByteArray contents) {
  qDebug("Download of application update file was completed with code '%d'.",
         status);

  switch (status) {
    case QNetworkReply::NoError:
      saveUpdateFile(contents);

      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok, tr("Downloaded successfully"),
                                   tr("Package was downloaded successfully."));
      m_btnUpdate->setText(tr("Install update"));
      m_btnUpdate->setEnabled(true);
      break;

    default:
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Error, tr("Error occured"),
                                   tr("Error occured during downloading of the package."));
      m_btnUpdate->setText(tr("Error occured"));
      break;
  }
}

void FormUpdate::startUpdate() {
  QString url_file;
  bool update_for_this_system = isUpdateForThisSystem();

  if (update_for_this_system) {
    url_file = m_updateInfo.m_urls.value(OS_ID).m_fileUrl;
  }
  else {
    url_file = APP_URL;
  }

  if (m_readyToInstall) {
    // Some package is downloaded and it can be installed
    // via self-update feature.
    close();

    qDebug("Preparing to launch external updater '%s'.", APP_UPDATER_EXECUTABLE);

    if (!QProcess::startDetached(APP_UPDATER_EXECUTABLE,
                                 QStringList() <<
                                 APP_VERSION <<
                                 m_updateInfo.m_availableVersion <<
                                 QDir::toNativeSeparators(qApp->applicationFilePath()) <<
                                 QDir::toNativeSeparators(m_updateFilePath))) {
      qDebug("External updater was not launched due to error.");

      if (SystemTrayIcon::isSystemTrayActivated()) {
        SystemTrayIcon::instance()->showMessage(tr("Cannot update application"),
                                                tr("Cannot launch external updater. Update application manually."),
                                                QSystemTrayIcon::Warning);
      }
      else {
        MessageBox::show(this,
                         QMessageBox::Warning,
                         tr("Cannot update application"),
                         tr("Cannot launch external updater. Update application manually."));
      }
    }
  }
  else if (update_for_this_system && isSelfUpdateSupported()) {
    // Nothing is downloaded yet, but update for this system
    // is available and self-update feature is present.

    if (m_downloader == NULL) {
      // Initialie downloader.
      m_downloader = new Downloader(this);

      connect(m_downloader, SIGNAL(progress(qint64,qint64)),
              this, SLOT(updateProgress(qint64,qint64)));
      connect(m_downloader, SIGNAL(completed(QNetworkReply::NetworkError,QByteArray)),
              this, SLOT(updateCompleted(QNetworkReply::NetworkError,QByteArray)));
    }

    m_btnUpdate->setText(tr("Downloading update..."));
    m_btnUpdate->setEnabled(false);

    m_downloader->downloadFile(url_file);

  } else {
    // Self-update and package are not available.
    if (!WebFactory::instance()->openUrlInExternalBrowser(url_file)) {
      if (SystemTrayIcon::isSystemTrayActivated()) {
        SystemTrayIcon::instance()->showMessage(tr("Cannot update application"),
                                                tr("Cannot navigate to installation file. Check new installation downloads "
                                                   "manually on project website."),
                                                QSystemTrayIcon::Warning);
      }
      else {
        MessageBox::show(this,
                         QMessageBox::Warning,
                         tr("Cannot update application"),
                         tr("Cannot navigate to installation file. Check new installation downloads "
                            "manually on project website."));
      }
    }
  }
}     
