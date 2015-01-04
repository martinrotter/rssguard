// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/formbackupdatabasesettings.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QDateTime>


FormBackupDatabaseSettings::FormBackupDatabaseSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormBackupDatabaseSettings) {
  m_ui->setupUi(this);
  m_ui->m_txtBackupName->lineEdit()->setPlaceholderText(tr("Common name for backup files"));

  setWindowIcon(qApp->icons()->fromTheme("document-export"));
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);

  connect(m_ui->m_checkBackupDatabase, SIGNAL(toggled(bool)), this, SLOT(checkOkButton()));
  connect(m_ui->m_checkBackupSettings, SIGNAL(toggled(bool)), this, SLOT(checkOkButton()));
  connect(m_ui->m_buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(performBackup()));
  connect(m_ui->m_txtBackupName->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkBackupNames(QString)));
  connect(m_ui->m_txtBackupName->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_btnSelectFolder, SIGNAL(clicked()), this, SLOT(selectFolder()));

  selectFolder(qApp->documentsFolderPath());  
  m_ui->m_txtBackupName->lineEdit()->setText(QString(APP_LOW_NAME) + "_" + QDateTime::currentDateTime().toString("yyyyMMddHHmm"));
  m_ui->m_lblResult->setStatus(WidgetWithStatus::Warning, tr("No operation executed yet."), tr("No operation executed yet."));

  if (qApp->database()->activeDatabaseDriver() != DatabaseFactory::SQLITE &&
      qApp->database()->activeDatabaseDriver() != DatabaseFactory::SQLITE_MEMORY) {
    m_ui->m_checkBackupDatabase->setDisabled(true);
  }
}

FormBackupDatabaseSettings::~FormBackupDatabaseSettings() {
  delete m_ui;
}

void FormBackupDatabaseSettings::performBackup() {
  if (qApp->backupDatabaseSettings(m_ui->m_checkBackupDatabase->isChecked(),
                                   m_ui->m_checkBackupSettings->isChecked(),
                                   m_ui->m_lblSelectFolder->label()->text(),
                                   m_ui->m_txtBackupName->lineEdit()->text())) {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::Ok,
                                 tr("Backup was created successfully and stored in target folder."),
                                 tr("Backup was created successfully."));
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::Error,
                                 tr("Backup failed, database and/or settings is probably not backed."),
                                 tr("Backup failed. Check the output folder if your database\nand/or "
                                    "settings were backed or not. Also make sure that target foder is writable."));
  }
}

void FormBackupDatabaseSettings::selectFolder(QString path) {
  if (path.isEmpty()) {
    path = QFileDialog::getExistingDirectory(this, tr("Select destionation folder"), m_ui->m_lblSelectFolder->label()->text());
  }

  if (!path.isEmpty()) {
    m_ui->m_lblSelectFolder->setStatus(WidgetWithStatus::Ok, QDir::toNativeSeparators(path),
                                       tr("Good destination folder is specified."));
  }
}

void FormBackupDatabaseSettings::checkBackupNames(const QString &name) {
  if (name.simplified().isEmpty()) {
    m_ui->m_txtBackupName->setStatus(WidgetWithStatus::Error, tr("Backup name cannot be empty."));
  }
  else {
    m_ui->m_txtBackupName->setStatus(WidgetWithStatus::Ok, tr("Backup name looks okay."));
  }
}

void FormBackupDatabaseSettings::checkOkButton() {
  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setDisabled(m_ui->m_txtBackupName->lineEdit()->text().simplified().isEmpty() ||
                                                               m_ui->m_lblSelectFolder->label()->text().simplified().isEmpty() ||
                                                               (!m_ui->m_checkBackupDatabase->isChecked() &&
                                                                !m_ui->m_checkBackupSettings->isChecked()));
}
