// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsdatabase.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"

SettingsDatabase::SettingsDatabase(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsDatabase) {
  m_ui->setupUi(this);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblDataStorageWarning, true);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblMysqlInfo, false);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblSqliteInMemoryWarnings, true);
  connect(m_ui->m_cmbDatabaseDriver, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_checkSqliteUseInMemoryDatabase, &QCheckBox::toggled, this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_txtMysqlDatabase->lineEdit(), &QLineEdit::textChanged, this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_txtMysqlHostname->lineEdit(), &QLineEdit::textChanged, this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_txtMysqlPassword->lineEdit(), &QLineEdit::textChanged, this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_checkUseTransactions, &QCheckBox::toggled, this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_txtMysqlUsername->lineEdit(), &QLineEdit::textChanged, this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_spinMysqlPort, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDatabase::dirtifySettings);
  connect(m_ui->m_cmbDatabaseDriver, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsDatabase::selectSqlBackend);
  connect(m_ui->m_checkMysqlShowPassword, &QCheckBox::toggled, this, &SettingsDatabase::switchMysqlPasswordVisiblity);
  connect(m_ui->m_txtMysqlUsername->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsDatabase::onMysqlUsernameChanged);
  connect(m_ui->m_txtMysqlHostname->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsDatabase::onMysqlHostnameChanged);
  connect(m_ui->m_txtMysqlPassword->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsDatabase::onMysqlPasswordChanged);
  connect(m_ui->m_txtMysqlDatabase->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsDatabase::onMysqlDatabaseChanged);
  connect(m_ui->m_btnMysqlTestSetup, &QPushButton::clicked, this, &SettingsDatabase::mysqlTestConnection);
  connect(m_ui->m_cmbDatabaseDriver, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsDatabase::requireRestart);
  connect(m_ui->m_checkSqliteUseInMemoryDatabase, &QCheckBox::toggled, this, &SettingsDatabase::requireRestart);
  connect(m_ui->m_spinMysqlPort, &QSpinBox::editingFinished, this, &SettingsDatabase::requireRestart);
  connect(m_ui->m_txtMysqlHostname->lineEdit(), &BaseLineEdit::textEdited, this, &SettingsDatabase::requireRestart);
  connect(m_ui->m_txtMysqlPassword->lineEdit(), &BaseLineEdit::textEdited, this, &SettingsDatabase::requireRestart);
  connect(m_ui->m_txtMysqlUsername->lineEdit(), &BaseLineEdit::textEdited, this, &SettingsDatabase::requireRestart);
}

SettingsDatabase::~SettingsDatabase() {
  delete m_ui;
}

void SettingsDatabase::mysqlTestConnection() {
  const DatabaseFactory::MySQLError error_code = qApp->database()->mysqlTestConnection(m_ui->m_txtMysqlHostname->lineEdit()->text(),
                                                                                       m_ui->m_spinMysqlPort->value(),
                                                                                       m_ui->m_txtMysqlDatabase->lineEdit()->text(),
                                                                                       m_ui->m_txtMysqlUsername->lineEdit()->text(),
                                                                                       m_ui->m_txtMysqlPassword->lineEdit()->text());
  const QString interpretation = qApp->database()->mysqlInterpretErrorCode(error_code);

  switch (error_code) {
    case DatabaseFactory::MySQLError::Ok:
    case DatabaseFactory::MySQLError::UnknownDatabase:
      m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::StatusType::Ok, interpretation, interpretation);
      break;

    default:
      m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::StatusType::Error, interpretation, interpretation);
      break;
  }
}

void SettingsDatabase::onMysqlHostnameChanged(const QString& new_hostname) {
  if (new_hostname.isEmpty()) {
    m_ui->m_txtMysqlHostname->setStatus(LineEditWithStatus::StatusType::Warning, tr("Hostname is empty."));
  }
  else {
    m_ui->m_txtMysqlHostname->setStatus(LineEditWithStatus::StatusType::Ok, tr("Hostname looks ok."));
  }
}

void SettingsDatabase::onMysqlUsernameChanged(const QString& new_username) {
  if (new_username.isEmpty()) {
    m_ui->m_txtMysqlUsername->setStatus(LineEditWithStatus::StatusType::Warning, tr("Username is empty."));
  }
  else {
    m_ui->m_txtMysqlUsername->setStatus(LineEditWithStatus::StatusType::Ok, tr("Username looks ok."));
  }
}

void SettingsDatabase::onMysqlPasswordChanged(const QString& new_password) {
  if (new_password.isEmpty()) {
    m_ui->m_txtMysqlPassword->setStatus(LineEditWithStatus::StatusType::Warning, tr("Password is empty."));
  }
  else {
    m_ui->m_txtMysqlPassword->setStatus(LineEditWithStatus::StatusType::Ok, tr("Password looks ok."));
  }
}

void SettingsDatabase::onMysqlDatabaseChanged(const QString& new_database) {
  if (new_database.isEmpty()) {
    m_ui->m_txtMysqlDatabase->setStatus(LineEditWithStatus::StatusType::Warning, tr("Working database is empty."));
  }
  else {
    m_ui->m_txtMysqlDatabase->setStatus(LineEditWithStatus::StatusType::Ok, tr("Working database is ok."));
  }
}

void SettingsDatabase::selectSqlBackend(int index) {
  const QString selected_db_driver = m_ui->m_cmbDatabaseDriver->itemData(index).toString();

  if (selected_db_driver == APP_DB_SQLITE_DRIVER) {
    m_ui->m_stackedDatabaseDriver->setCurrentIndex(0);
  }
  else if (selected_db_driver == APP_DB_MYSQL_DRIVER) {
    m_ui->m_stackedDatabaseDriver->setCurrentIndex(1);
  }
  else {
    qWarningNN << LOGSEC_GUI
               << "GUI for given database driver '"
               << selected_db_driver
               << "' is not available.";
  }
}

void SettingsDatabase::switchMysqlPasswordVisiblity(bool visible) {
  m_ui->m_txtMysqlPassword->lineEdit()->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
}

void SettingsDatabase::loadSettings() {
  onBeginLoadSettings();
  m_ui->m_checkUseTransactions->setChecked(qApp->settings()->value(GROUP(Database), SETTING(Database::UseTransactions)).toBool());
  m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::StatusType::Information, tr("No connection test triggered so far."),
                                        tr("You did not executed any connection test yet."));

  // Load SQLite.
  m_ui->m_cmbDatabaseDriver->addItem(qApp->database()->humanDriverName(DatabaseFactory::UsedDriver::SQLITE), APP_DB_SQLITE_DRIVER);

  // Load in-memory database status.
  m_ui->m_checkSqliteUseInMemoryDatabase->setChecked(settings()->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool());

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    onMysqlHostnameChanged(QString());
    onMysqlUsernameChanged(QString());
    onMysqlPasswordChanged(QString());
    onMysqlDatabaseChanged(QString());

    // Load MySQL.
    m_ui->m_cmbDatabaseDriver->addItem(qApp->database()->humanDriverName(DatabaseFactory::UsedDriver::MYSQL), APP_DB_MYSQL_DRIVER);

    // Setup placeholders.
    m_ui->m_txtMysqlHostname->lineEdit()->setPlaceholderText(tr("Hostname of your MySQL server"));
    m_ui->m_txtMysqlUsername->lineEdit()->setPlaceholderText(tr("Username to login with"));
    m_ui->m_txtMysqlPassword->lineEdit()->setPlaceholderText(tr("Password for your username"));
    m_ui->m_txtMysqlDatabase->lineEdit()->setPlaceholderText(tr("Working database which you have full access to."));
    m_ui->m_txtMysqlHostname->lineEdit()->setText(settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
    m_ui->m_txtMysqlUsername->lineEdit()->setText(settings()->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
    m_ui->m_txtMysqlPassword->lineEdit()->setText(settings()->password(GROUP(Database),
                                                                       SETTING(Database::MySQLPassword)).toString());
    m_ui->m_txtMysqlDatabase->lineEdit()->setText(settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString());
    m_ui->m_spinMysqlPort->setValue(settings()->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());
    m_ui->m_checkMysqlShowPassword->setChecked(false);
  }

  int index_current_backend = m_ui->m_cmbDatabaseDriver->findData(settings()->value(GROUP(Database),
                                                                                    SETTING(Database::ActiveDriver)).toString());

  if (index_current_backend >= 0) {
    m_ui->m_cmbDatabaseDriver->setCurrentIndex(index_current_backend);
  }

  onEndLoadSettings();
}

void SettingsDatabase::saveSettings() {
  onBeginSaveSettings();

  // Setup in-memory database status.
  const bool original_inmemory = settings()->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool();
  const bool new_inmemory = m_ui->m_checkSqliteUseInMemoryDatabase->isChecked();

  qApp->settings()->setValue(GROUP(Database), Database::UseTransactions, m_ui->m_checkUseTransactions->isChecked());

  // Save data storage settings.
  QString original_db_driver = settings()->value(GROUP(Database), SETTING(Database::ActiveDriver)).toString();
  QString selected_db_driver = m_ui->m_cmbDatabaseDriver->itemData(m_ui->m_cmbDatabaseDriver->currentIndex()).toString();

  // Save SQLite.
  settings()->setValue(GROUP(Database), Database::UseInMemory, new_inmemory);

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    // Save MySQL.
    settings()->setValue(GROUP(Database), Database::MySQLHostname, m_ui->m_txtMysqlHostname->lineEdit()->text());
    settings()->setValue(GROUP(Database), Database::MySQLUsername, m_ui->m_txtMysqlUsername->lineEdit()->text());
    settings()->setPassword(GROUP(Database), Database::MySQLPassword, m_ui->m_txtMysqlPassword->lineEdit()->text());
    settings()->setValue(GROUP(Database), Database::MySQLDatabase, m_ui->m_txtMysqlDatabase->lineEdit()->text());
    settings()->setValue(GROUP(Database), Database::MySQLPort, m_ui->m_spinMysqlPort->value());
  }

  settings()->setValue(GROUP(Database), Database::ActiveDriver, selected_db_driver);

  if (original_db_driver != selected_db_driver || original_inmemory != new_inmemory) {
    requireRestart();
  }

  onEndSaveSettings();
}
