// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formdatabasecleanup.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QPushButton>

FormDatabaseCleanup::FormDatabaseCleanup(QWidget* parent) : QDialog(parent), m_ui(new Ui::FormDatabaseCleanup), m_cleaner(nullptr) {
  m_ui->setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("edit-clear")));

  connect(m_ui->m_spinDays, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FormDatabaseCleanup::updateDaysSuffix);
  connect(m_ui->m_btnBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &FormDatabaseCleanup::startPurging);
  connect(this, &FormDatabaseCleanup::purgeRequested, &m_cleaner, &DatabaseCleaner::purgeDatabaseData);
  connect(&m_cleaner, &DatabaseCleaner::purgeStarted, this, &FormDatabaseCleanup::onPurgeStarted);
  connect(&m_cleaner, &DatabaseCleaner::purgeProgress, this, &FormDatabaseCleanup::onPurgeProgress);
  connect(&m_cleaner, &DatabaseCleaner::purgeFinished, this, &FormDatabaseCleanup::onPurgeFinished);

  m_ui->m_spinDays->setValue(DEFAULT_DAYS_TO_DELETE_MSG);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Information, tr("I am ready."), tr("I am ready."));

  loadDatabaseInfo();
}

void FormDatabaseCleanup::closeEvent(QCloseEvent* event) {
  if (m_ui->m_progressBar->isEnabled()) {
    event->ignore();
  }
  else {
    QDialog::closeEvent(event);
  }
}

void FormDatabaseCleanup::keyPressEvent(QKeyEvent* event) {
  if (m_ui->m_progressBar->isEnabled()) {
    event->ignore();
  }
  else {
    QDialog::keyPressEvent(event);
  }
}

void FormDatabaseCleanup::updateDaysSuffix(int number) {
  m_ui->m_spinDays->setSuffix(tr(" day(s)", nullptr, number));
}

void FormDatabaseCleanup::startPurging() {
  CleanerOrders orders;

  orders.m_removeRecycleBin = m_ui->m_checkRemoveRecycleBin->isChecked();
  orders.m_removeOldMessages = m_ui->m_checkRemoveOldMessages->isChecked();
  orders.m_barrierForRemovingOldMessagesInDays = m_ui->m_spinDays->value();
  orders.m_removeReadMessages = m_ui->m_checkRemoveReadMessages->isChecked();
  orders.m_shrinkDatabase = m_ui->m_checkShrink->isEnabled() && m_ui->m_checkShrink->isChecked();
  orders.m_removeStarredMessages = m_ui->m_checkRemoveStarredMessages->isChecked();

  emit purgeRequested(orders);
}

void FormDatabaseCleanup::onPurgeStarted() {
  m_ui->m_progressBar->setValue(0);
  m_ui->m_progressBar->setEnabled(true);
  m_ui->m_btnBox->setEnabled(false);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Information, tr("Database cleanup is running."),
                               tr("Database cleanup is running."));
}

void FormDatabaseCleanup::onPurgeProgress(int progress, const QString& description) {
  m_ui->m_progressBar->setValue(progress);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Information, description, description);
}

void FormDatabaseCleanup::onPurgeFinished(bool finished) {
  m_ui->m_progressBar->setEnabled(false);
  m_ui->m_progressBar->setValue(0);
  m_ui->m_btnBox->setEnabled(true);

  if (finished) {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                 tr("Database cleanup is completed."),
                                 tr("Database cleanup is completed."));
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, tr("Database cleanup failed."), tr("Database cleanup failed."));
  }

  loadDatabaseInfo();
}

void FormDatabaseCleanup::loadDatabaseInfo() {
  qint64 file_size = qApp->database()->getDatabaseFileSize();
  qint64 data_size = qApp->database()->getDatabaseDataSize();
  QString file_size_str = file_size > 0 ? QString::number(file_size / 1000000.0) + QL1S(" MB") : tr("unknown");
  QString data_size_str = data_size > 0 ? QString::number(data_size / 1000000.0) + QL1S(" MB") : tr("unknown");

  m_ui->m_txtFileSize->setText(tr("file: %1, data: %2").arg(file_size_str, data_size_str));
  m_ui->m_txtDatabaseType->setText(qApp->database()->humanDriverName(qApp->database()->activeDatabaseDriver()));
  m_ui->m_checkShrink->setChecked(m_ui->m_checkShrink->isEnabled());
}
