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
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"

#include <QNetworkReply>
#include <QDesktopServices>
#include <QProcess>

#if defined(Q_OS_WIN)
#include "qt_windows.h"
#endif

FormUpdate::FormUpdate(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormUpdate) {
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

bool FormUpdate::isUpdateForThisSystem() {
  return m_updateInfo.m_urls.keys().contains(OS_ID);
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

    if (update.first.m_availableVersion >= APP_VERSION) {
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
                                   tr("New release available."),
                                   tr("This is new version which can be\ndownloaded and installed."));
      m_btnUpdate->setEnabled(true);
      m_btnUpdate->setToolTip(isUpdateForThisSystem() ?
                                tr("Download installation file for your OS.") :
                                tr("Installation file is not available directly.\n"
                                   "Go to application website to obtain it manually."));
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

void FormUpdate::startUpdate() {
  QString url_file;

  if (isUpdateForThisSystem()) {
    url_file = m_updateInfo.m_urls.value(OS_ID).m_fileUrl;
  }
  else {
    url_file = APP_URL;
  }

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
  // On Windows/OS2 we can update the application right away.
  // Download the files.
  QByteArray output;
  QNetworkReply::NetworkError download_result = NetworkFactory::downloadFile(url_file,
                                                                             10 * DOWNLOAD_TIMEOUT,
                                                                             output);

#if QT_VERSION >= 0x050000
  QString temp_directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
  QString temp_directory = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

  if (!temp_directory.isEmpty()) {
    QString output_file_name = url_file.mid(url_file.lastIndexOf('/') + 1);
    QFile output_file(temp_directory + QDir::separator() + output_file_name);

    if (output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      output_file.write(output);
      output_file.flush();
      output_file.close();

      // TODO: spustit updater
      // pouzit qprocess, nebo neco multiplatformniho
      // nebo z quiterss shellexecuter
      // program obcas pada, to je mozna zpusobeny tim
      // ze je otevreny modalni okno.
      close();

#if defined(Q_OS_WIN32)
      ShellExecute(0,
                   0,
                   (wchar_t *) QString("updater.exe").utf16(),
                   (wchar_t *) QString("\"%1\" \"%2\" \"%3\"").arg(temp_directory,
                                                                   qApp->applicationFilePath(),
                                                                   output_file.fileName()).utf16(),
                   0,
                   SW_SHOWNORMAL);
#elif defined(Q_OS_OS2)

#endif
      // TODO: vetev pro osn
    }
    else {
      // TODO: chyba - nelze zapisovat do souboru
    }

  }
  else {
    // TODO: chyba - nelze ulozit soubor.
  }

#else
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
#endif
}
