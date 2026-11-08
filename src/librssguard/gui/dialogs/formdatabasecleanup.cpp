// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formdatabasecleanup.h"

#include "database/databasefactory.h"
#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QPushButton>

FormDatabaseCleanup::FormDatabaseCleanup(QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormDatabaseCleanup), m_cleaner(nullptr) {
  m_ui->setupUi(this);

  setObjectName(QSL("form_db_cleanup"));

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("edit-clear")));

#if defined(Q_OS_WIN)
  m_ui->m_helpShrink->setHelpText(tr("Why can the database file stay large?"),
                                  tr("For faster database access, RSS Guard uses a Windows feature that can prevent "
                                     "SQLite database files from shrinking on disk. The \"Total data size\" shown here "
                                     "may still decrease after optimization. The unused space will be reused as new "
                                     "articles are saved."),
                                  false);
  m_ui->m_helpShrink->setVisible(qApp->database()->driver()->driverType() == DatabaseDriver::DriverType::SQLite);
#else
  m_ui->m_helpShrink->hide();
#endif

  connect(m_ui->m_spinDays, QOverload<int>::of(&QSpinBox::valueChanged), this, &FormDatabaseCleanup::updateDaysSuffix);
  connect(m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Ok),
          &QPushButton::clicked,
          this,
          &FormDatabaseCleanup::startPurging);
  connect(this, &FormDatabaseCleanup::purgeRequested, &m_cleaner, &DatabaseCleaner::purgeDatabaseData);
  connect(&m_cleaner, &DatabaseCleaner::purgeStarted, this, &FormDatabaseCleanup::onPurgeStarted);
  connect(&m_cleaner, &DatabaseCleaner::purgeProgress, this, &FormDatabaseCleanup::onPurgeProgress);
  connect(&m_cleaner, &DatabaseCleaner::purgeFinished, this, &FormDatabaseCleanup::onPurgeFinished);
  connect(&m_cleaner, &DatabaseCleaner::purgeFailed, this, &FormDatabaseCleanup::onPurgeFailed);

  const CleanerOrders orders = DatabaseCleaner::loadSettings();

  m_ui->m_checkRemoveReadMessages->setChecked(orders.m_removeReadMessages);
  m_ui->m_checkShrink->setChecked(orders.m_shrinkDatabase);
  m_ui->m_checkRemoveOldMessages->setChecked(orders.m_removeOldMessages);
  m_ui->m_checkRemoveRecycleBin->setChecked(orders.m_removeRecycleBin);
  m_ui->m_checkRemoveStarredMessages->setChecked(orders.m_removeStarredMessages);
  m_ui->m_spinDays->setValue(orders.m_barrierForRemovingOldMessagesInDays);
  updateDaysSuffix(m_ui->m_spinDays->value());
  m_ui->m_cmbAutomaticCleanup->setCurrentIndex(int(DatabaseCleaner::automaticCleanupTiming()));
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Information, tr("I am ready."), tr("I am ready."));

  loadDatabaseInfo();
}

void FormDatabaseCleanup::closeEvent(QCloseEvent* event) {
  if (!m_ui->m_btnBox->isEnabled()) {
    event->ignore();
  }
  else {
    QDialog::closeEvent(event);
  }
}

void FormDatabaseCleanup::keyPressEvent(QKeyEvent* event) {
  if (!m_ui->m_btnBox->isEnabled()) {
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
  const CleanerOrders orders = cleanupOrders();

  DatabaseCleaner::saveSettings(orders, DatabaseCleaner::Timing(m_ui->m_cmbAutomaticCleanup->currentIndex()));
  emit purgeRequested(orders);
}

CleanerOrders FormDatabaseCleanup::cleanupOrders() const {
  CleanerOrders orders;

  orders.m_removeRecycleBin = m_ui->m_checkRemoveRecycleBin->isChecked();
  orders.m_removeOldMessages = m_ui->m_checkRemoveOldMessages->isChecked();
  orders.m_barrierForRemovingOldMessagesInDays = m_ui->m_spinDays->value();
  orders.m_removeReadMessages = m_ui->m_checkRemoveReadMessages->isChecked();
  orders.m_shrinkDatabase = m_ui->m_checkShrink->isEnabled() && m_ui->m_checkShrink->isChecked();
  orders.m_removeStarredMessages = m_ui->m_checkRemoveStarredMessages->isChecked();

  return orders;
}

void FormDatabaseCleanup::onPurgeStarted() {
  m_ui->m_progressBar->setValue(0);
  m_ui->m_btnBox->setEnabled(false);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Information,
                               tr("Database cleanup is running."),
                               tr("Database cleanup is running."));
}

void FormDatabaseCleanup::onPurgeProgress(int progress, const QString& description) {
  m_ui->m_progressBar->setValue(progress);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Information, description, description);
}

void FormDatabaseCleanup::onPurgeFinished() {
  m_ui->m_progressBar->setValue(100);
  m_ui->m_btnBox->setEnabled(true);

  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok,
                               tr("Database cleanup is completed."),
                               tr("Database cleanup is completed."));

  loadDatabaseInfo();
}

void FormDatabaseCleanup::onPurgeFailed(const QString& error) {
  m_ui->m_btnBox->setEnabled(true);
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, error, tr("Database cleanup failed."));
}

void FormDatabaseCleanup::loadDatabaseInfo() {
  qint64 data_size = qApp->database()->driver()->databaseDataSize();
  QString data_size_str = data_size > 0 ? QString::number(data_size / 1000000.0) + QL1S(" MB") : tr("unknown");

  m_ui->m_txtFileSize->setText(data_size_str);
  m_ui->m_txtDatabaseType->setText(qApp->database()->driver()->humanDriverType());
}

void FormDatabaseCleanup::hideEvent(QHideEvent* event) {
  DatabaseCleaner::saveSettings(cleanupOrders(), DatabaseCleaner::Timing(m_ui->m_cmbAutomaticCleanup->currentIndex()));
  QDialog::hideEvent(event);
}
