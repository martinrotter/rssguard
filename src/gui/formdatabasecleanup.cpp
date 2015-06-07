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

#include "gui/formdatabasecleanup.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/databasefactory.h"

#include <QCloseEvent>


FormDatabaseCleanup::FormDatabaseCleanup(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormDatabaseCleanup), m_cleaner(NULL) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme("cleanup-database"));

  connect(m_ui->m_spinDays, SIGNAL(valueChanged(int)), this, SLOT(updateDaysSuffix(int)));
  m_ui->m_spinDays->setValue(DEFAULT_DAYS_TO_DELETE_MSG);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::Information, tr("I am ready."), tr("I am ready."));
  loadDatabaseInfo();
}

FormDatabaseCleanup::~FormDatabaseCleanup() {
  delete m_ui;
}

void FormDatabaseCleanup::setCleaner(DatabaseCleaner *cleaner) {
  if (m_cleaner != NULL) {
    disconnect(this, 0, m_cleaner, 0);
    disconnect(m_cleaner, 0, this, 0);
  }

  m_cleaner = cleaner;

  connect(m_ui->m_btnBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(startPurging()));
  connect(this, SIGNAL(purgeRequested(CleanerOrders)), m_cleaner, SLOT(purgeDatabaseData(CleanerOrders)));
  connect(m_cleaner, SIGNAL(purgeStarted()), this, SLOT(onPurgeStarted()));
  connect(m_cleaner, SIGNAL(purgeProgress(int,QString)), this, SLOT(onPurgeProgress(int,QString)));
  connect(m_cleaner, SIGNAL(purgeFinished(bool)), this, SLOT(onPurgeFinished(bool)));
}

void FormDatabaseCleanup::closeEvent(QCloseEvent *event) {
  if (m_ui->m_progressBar->isEnabled()) {
    event->ignore();
  }
  else {
    QDialog::closeEvent(event);
  }
}

void FormDatabaseCleanup::keyPressEvent(QKeyEvent *event) {
  if (m_ui->m_progressBar->isEnabled()) {
    event->ignore();
  }
  else {
    QDialog::keyPressEvent(event);
  }
}

void FormDatabaseCleanup::updateDaysSuffix(int number) {
  m_ui->m_spinDays->setSuffix(tr(" day(s)", 0, number));
}

void FormDatabaseCleanup::startPurging() {
  CleanerOrders orders;

  orders.m_removeOldMessages = m_ui->m_checkRemoveOldMessages->isChecked();
  orders.m_barrierForRemovingOldMessagesInDays = m_ui->m_spinDays->value();
  orders.m_removeReadMessages = m_ui->m_checkRemoveReadMessages->isChecked();
  orders.m_shrinkDatabase = m_ui->m_checkShrink->isEnabled() && m_ui->m_checkShrink->isChecked();

  emit purgeRequested(orders);
}

void FormDatabaseCleanup::onPurgeStarted() {
  m_ui->m_progressBar->setValue(0);
  m_ui->m_progressBar->setEnabled(true);
  m_ui->m_btnBox->setEnabled(false);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::Information, tr("Database cleanup is running."), tr("Database cleanup is running."));
}

void FormDatabaseCleanup::onPurgeProgress(int progress, const QString &description) {
  m_ui->m_progressBar->setValue(progress);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::Information, description, description);
}

void FormDatabaseCleanup::onPurgeFinished(bool finished) {
  m_ui->m_progressBar->setEnabled(false);
  m_ui->m_progressBar->setValue(0);
  m_ui->m_btnBox->setEnabled(true);

  if (finished) {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::Ok, tr("Database cleanup is completed."), tr("Database cleanup is completed."));
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::Error, tr("Database cleanup failed."), tr("Database cleanup failed."));
  }

  loadDatabaseInfo();
}

void FormDatabaseCleanup::loadDatabaseInfo() {
  qint64 db_size = qApp->database()->getDatabaseSize();

  if (db_size > 0) {
    m_ui->m_txtFileSize->setText(QString::number(db_size / 1000000.0) + " MB");
  }
  else {
    m_ui->m_txtFileSize->setText("-");
  }

  m_ui->m_txtDatabaseType->setText(qApp->database()->humanDriverName(qApp->database()->activeDatabaseDriver()));
  m_ui->m_checkShrink->setEnabled(qApp->database()->activeDatabaseDriver() == DatabaseFactory::SQLITE);
  m_ui->m_checkShrink->setChecked(m_ui->m_checkShrink->isEnabled());
}
