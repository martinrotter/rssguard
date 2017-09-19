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

#include "gui/settings/settingsdownloads.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/downloadmanager.h"

#include <QFileDialog>

SettingsDownloads::SettingsDownloads(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsDownloads) {
  m_ui->setupUi(this);
  connect(m_ui->m_checkOpenManagerWhenDownloadStarts, &QCheckBox::toggled, this, &SettingsDownloads::dirtifySettings);
  connect(m_ui->m_txtDownloadsTargetDirectory, &QLineEdit::textChanged, this, &SettingsDownloads::dirtifySettings);
  connect(m_ui->m_rbDownloadsAskEachFile, &QRadioButton::toggled, this, &SettingsDownloads::dirtifySettings);
  connect(m_ui->m_btnDownloadsTargetDirectory, &QPushButton::clicked, this, &SettingsDownloads::selectDownloadsDirectory);
}

SettingsDownloads::~SettingsDownloads() {
  delete m_ui;
}

void SettingsDownloads::selectDownloadsDirectory() {
  const QString target_directory = QFileDialog::getExistingDirectory(this,
                                                                     tr("Select downloads target directory"),
                                                                     m_ui->m_txtDownloadsTargetDirectory->text()
                                                                     );

  if (!target_directory.isEmpty()) {
    m_ui->m_txtDownloadsTargetDirectory->setText(QDir::toNativeSeparators(target_directory));
  }
}

void SettingsDownloads::loadSettings() {
  onBeginLoadSettings();
  m_ui->m_checkOpenManagerWhenDownloadStarts->setChecked(settings()->value(GROUP(Downloads),
                                                                           SETTING(Downloads::ShowDownloadsWhenNewDownloadStarts)).toBool());
  m_ui->m_txtDownloadsTargetDirectory->setText(QDir::toNativeSeparators(settings()->value(GROUP(Downloads),
                                                                                          SETTING(Downloads::TargetDirectory)).toString()));
  m_ui->m_rbDownloadsAskEachFile->setChecked(settings()->value(GROUP(Downloads),
                                                               SETTING(Downloads::AlwaysPromptForFilename)).toBool());
  onEndLoadSettings();
}

void SettingsDownloads::saveSettings() {
  onBeginSaveSettings();
  settings()->setValue(GROUP(Downloads), Downloads::ShowDownloadsWhenNewDownloadStarts,
                       m_ui->m_checkOpenManagerWhenDownloadStarts->isChecked());
  settings()->setValue(GROUP(Downloads), Downloads::TargetDirectory, m_ui->m_txtDownloadsTargetDirectory->text());
  settings()->setValue(GROUP(Downloads), Downloads::AlwaysPromptForFilename, m_ui->m_rbDownloadsAskEachFile->isChecked());
  qApp->downloadManager()->setDownloadDirectory(m_ui->m_txtDownloadsTargetDirectory->text());
  onEndSaveSettings();
}
