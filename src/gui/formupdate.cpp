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

#include "core/defs.h"
#include "core/systemfactory.h"
#include "core/networkfactory.h"
#include "gui/iconthemefactory.h"

#if !defined(Q_OS_WIN)
#include "gui/messagebox.h"
#endif

#include <QNetworkReply>


FormUpdate::FormUpdate(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormUpdate) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::instance()->fromTheme("application-about"));

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  m_ui->m_lblCurrentRelease->setText(APP_VERSION);
  checkForUpdates();
}

FormUpdate::~FormUpdate() {
  delete m_ui;
}

// TODO: tady v update nacist do m_lblSupportedPlatforms
// seznam platform ktery danej release podporuje oddelenej carkama
// treba "Windows, OS2" atp atp.
// ten combobox se statusem previst na normalni combobox
// asi. jednotlivy URL souborů pro danej release
// sou dostupny v qhashi podle klice podle OS.

void FormUpdate::checkForUpdates() {
  QPair<UpdateInfo, QNetworkReply::NetworkError> update = SystemFactory::instance()->checkForUpdates();

  if (update.second != QNetworkReply::NoError) {
    //: Uknown release.
    m_ui->m_lblAvailableRelease->setText(tr("unknown"));
    m_ui->m_txtChanges->clear();
    m_ui->m_lblStatus->setStatus(WidgetWithStatus::Error,
                                 tr("Error: '%1'.").arg(NetworkFactory::networkErrorText(update.second)),
                                 tr("List with updates was "
                                    "not\ndownloaded successfully."));
  }
  else {
    m_ui->m_lblAvailableRelease->setText(update.first.m_availableVersion);
    m_ui->m_txtChanges->setText(update.first.m_changes);

    if (update.first.m_availableVersion > APP_VERSION) {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
                                   tr("New release available."),
                                   tr("This is new version which can be\ndownloaded and installed."));
      // TODO: Display "update" button if
      // URL of file for current platform (Windows or OS2)
      // is available.
      // TODO: Tady po stisku update tlacitka se provede
      // stazeni archivu do tempu.
#else
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
                                   tr("New release available."),
                                   tr("This is new version. Upgrade to it manually or via your system package manager."));
#endif
    }
    else {
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Warning,
                                   tr("No new release available."),
                                   tr("This release is not newer than\ncurrently installed one."));
    }
  }
}
