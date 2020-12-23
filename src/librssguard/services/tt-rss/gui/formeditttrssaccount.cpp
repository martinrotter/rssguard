// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/gui/formeditttrssaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"

FormEditTtRssAccount::FormEditTtRssAccount(QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormEditTtRssAccount), m_editableRoot(nullptr) {
  m_ui->setupUi(this);
  m_btnOk = m_ui->m_buttonBox->button(QDialogButtonBox::Ok);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("tt-rss")));

  m_ui->m_lblTestResult->label()->setWordWrap(true);
  m_ui->m_lblServerSideUpdateInformation->setText(tr("Leaving this option on causes that updates "
                                                     "of feeds will be probably much slower and may time-out often."));
  m_ui->m_lblDescription->setText(tr("Note that at least API level %1 is required.").arg(TTRSS_MINIMAL_API_LEVEL));
  m_ui->m_txtHttpUsername->lineEdit()->setPlaceholderText(tr("HTTP authentication username"));
  m_ui->m_txtHttpPassword->lineEdit()->setPlaceholderText(tr("HTTP authentication password"));
  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your TT-RSS account"));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your TT-RSS account"));
  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("URL of your TT-RSS instance WITHOUT trailing \"/api/\" string"));
  m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                   tr("No test done yet."),
                                   tr("Here, results of connection test are shown."));

  GuiUtilities::setLabelAsNotice(*m_ui->m_lblDescription, false);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblServerSideUpdateInformation, false);

  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_checkServerSideUpdate);
  setTabOrder(m_ui->m_checkServerSideUpdate, m_ui->m_txtUsername->lineEdit());
  setTabOrder(m_ui->m_txtUsername->lineEdit(), m_ui->m_txtPassword->lineEdit());
  setTabOrder(m_ui->m_txtPassword->lineEdit(), m_ui->m_checkShowPassword);
  setTabOrder(m_ui->m_checkShowPassword, m_ui->m_gbHttpAuthentication);
  setTabOrder(m_ui->m_gbHttpAuthentication, m_ui->m_txtHttpUsername->lineEdit());
  setTabOrder(m_ui->m_txtHttpUsername->lineEdit(), m_ui->m_txtHttpPassword->lineEdit());
  setTabOrder(m_ui->m_txtHttpPassword->lineEdit(), m_ui->m_checkShowHttpPassword);
  setTabOrder(m_ui->m_checkShowHttpPassword, m_ui->m_btnTestSetup);
  setTabOrder(m_ui->m_btnTestSetup, m_ui->m_buttonBox);

  connect(m_ui->m_checkShowPassword, &QCheckBox::toggled, this, &FormEditTtRssAccount::displayPassword);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormEditTtRssAccount::onClickedOk);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &FormEditTtRssAccount::onClickedCancel);
  connect(m_ui->m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::onPasswordChanged);
  connect(m_ui->m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::onUsernameChanged);
  connect(m_ui->m_txtHttpPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::onHttpPasswordChanged);
  connect(m_ui->m_txtHttpUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::onHttpUsernameChanged);
  connect(m_ui->m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::onUrlChanged);
  connect(m_ui->m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::checkOkButton);
  connect(m_ui->m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::checkOkButton);
  connect(m_ui->m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditTtRssAccount::checkOkButton);
  connect(m_ui->m_btnTestSetup, &QPushButton::clicked, this, &FormEditTtRssAccount::performTest);
  connect(m_ui->m_gbHttpAuthentication, &QGroupBox::toggled, this, &FormEditTtRssAccount::onHttpPasswordChanged);
  connect(m_ui->m_gbHttpAuthentication, &QGroupBox::toggled, this, &FormEditTtRssAccount::onHttpUsernameChanged);
  connect(m_ui->m_checkShowHttpPassword, &QCheckBox::toggled, this, &FormEditTtRssAccount::displayHttpPassword);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
  onHttpPasswordChanged();
  onHttpUsernameChanged();
  checkOkButton();
  displayPassword(false);
  displayHttpPassword(false);
}

FormEditTtRssAccount::~FormEditTtRssAccount() = default;

TtRssServiceRoot* FormEditTtRssAccount::execForCreate() {
  setWindowTitle(tr("Add new Tiny Tiny RSS account"));
  exec();
  return m_editableRoot;
}

void FormEditTtRssAccount::execForEdit(TtRssServiceRoot* existing_root) {
  setWindowTitle(tr("Edit existing Tiny Tiny RSS account"));
  m_editableRoot = existing_root;
  m_ui->m_gbHttpAuthentication->setChecked(existing_root->network()->authIsUsed());
  m_ui->m_txtHttpPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_ui->m_txtHttpUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_ui->m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_ui->m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_ui->m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_ui->m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());
  m_ui->m_checkDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
  exec();
}

void FormEditTtRssAccount::displayPassword(bool display) {
  m_ui->m_txtPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void FormEditTtRssAccount::displayHttpPassword(bool display) {
  m_ui->m_txtHttpPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void FormEditTtRssAccount::performTest() {
  TtRssNetworkFactory factory;

  factory.setUsername(m_ui->m_txtUsername->lineEdit()->text());
  factory.setPassword(m_ui->m_txtPassword->lineEdit()->text());
  factory.setUrl(m_ui->m_txtUrl->lineEdit()->text());
  factory.setAuthIsUsed(m_ui->m_gbHttpAuthentication->isChecked());
  factory.setAuthUsername(m_ui->m_txtHttpUsername->lineEdit()->text());
  factory.setAuthPassword(m_ui->m_txtHttpPassword->lineEdit()->text());
  factory.setForceServerSideUpdate(m_ui->m_checkServerSideUpdate->isChecked());

  TtRssLoginResponse result = factory.login();

  if (result.isLoaded()) {
    if (result.hasError()) {
      QString error = result.error();

      if (error == TTRSS_API_DISABLED) {
        m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                         tr("API access on selected server is not enabled."),
                                         tr("API access on selected server is not enabled."));
      }
      else if (error == TTRSS_LOGIN_ERROR) {
        m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                         tr("Entered credentials are incorrect."),
                                         tr("Entered credentials are incorrect."));
      }
      else {
        m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                         tr("Other error occurred, contact developers."),
                                         tr("Other error occurred, contact developers."));
      }
    }
    else if (result.apiLevel() < TTRSS_MINIMAL_API_LEVEL) {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Installed version: %1, required at least: %2.").arg(QString::number(result.apiLevel()),
                                                                                               QString::number(TTRSS_MINIMAL_API_LEVEL)),
                                       tr("Selected Tiny Tiny RSS server is running unsupported version of API."));
    }
    else {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr("Installed version: %1, required at least: %2.").arg(QString::number(result.apiLevel()),
                                                                                               QString::number(TTRSS_MINIMAL_API_LEVEL)),
                                       tr("Tiny Tiny RSS server is okay."));
    }
  }
  else if (factory.lastError()  != QNetworkReply::NoError) {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                     tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(factory.lastError())),
                                     tr("Network error, have you entered correct Tiny Tiny RSS API endpoint and password?"));
  }
  else {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                     tr("Unspecified error, did you enter correct URL?"),
                                     tr("Unspecified error, did you enter correct URL?"));
  }
}

void FormEditTtRssAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new TtRssServiceRoot();
    editing_account = false;
  }

  m_editableRoot->network()->setUrl(m_ui->m_txtUrl->lineEdit()->text());
  m_editableRoot->network()->setUsername(m_ui->m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setPassword(m_ui->m_txtPassword->lineEdit()->text());
  m_editableRoot->network()->setAuthIsUsed(m_ui->m_gbHttpAuthentication->isChecked());
  m_editableRoot->network()->setAuthUsername(m_ui->m_txtHttpUsername->lineEdit()->text());
  m_editableRoot->network()->setAuthPassword(m_ui->m_txtHttpPassword->lineEdit()->text());
  m_editableRoot->network()->setForceServerSideUpdate(m_ui->m_checkServerSideUpdate->isChecked());
  m_editableRoot->network()->setDownloadOnlyUnreadMessages(m_ui->m_checkDownloadOnlyUnreadMessages->isChecked());

  m_editableRoot->saveAccountDataToDatabase();
  accept();

  if (editing_account) {
    m_editableRoot->network()->logout();
    m_editableRoot->completelyRemoveAllData();
    m_editableRoot->syncIn();
  }
}

void FormEditTtRssAccount::onClickedCancel() {
  reject();
}

void FormEditTtRssAccount::onUsernameChanged() {
  const QString username = m_ui->m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void FormEditTtRssAccount::onPasswordChanged() {
  const QString password = m_ui->m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::StatusType::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("Password is okay."));
  }
}

void FormEditTtRssAccount::onHttpUsernameChanged() {
  const bool is_username_ok = !m_ui->m_gbHttpAuthentication->isChecked() || !m_ui->m_txtHttpUsername->lineEdit()->text().isEmpty();

  m_ui->m_txtHttpUsername->setStatus(is_username_ok ?
                                     LineEditWithStatus::StatusType::Ok :
                                     LineEditWithStatus::StatusType::Warning,
                                     is_username_ok ?
                                     tr("Username is ok or it is not needed.") :
                                     tr("Username is empty."));
}

void FormEditTtRssAccount::onHttpPasswordChanged() {
  const bool is_username_ok = !m_ui->m_gbHttpAuthentication->isChecked() || !m_ui->m_txtHttpPassword->lineEdit()->text().isEmpty();

  m_ui->m_txtHttpPassword->setStatus(is_username_ok ?
                                     LineEditWithStatus::StatusType::Ok :
                                     LineEditWithStatus::StatusType::Warning,
                                     is_username_ok ?
                                     tr("Password is ok or it is not needed.") :
                                     tr("Password is empty."));
}

void FormEditTtRssAccount::onUrlChanged() {
  const QString url = m_ui->m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("URL cannot be empty."));
  }
  else if (url.endsWith(QL1S("/api/")) || url.endsWith(QL1S("/api"))) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::StatusType::Warning, tr("URL should NOT end with \"/api/\"."));
  }
  else {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is okay."));
  }
}

void FormEditTtRssAccount::checkOkButton() {
  m_btnOk->setEnabled(!m_ui->m_txtUsername->lineEdit()->text().isEmpty() &&
                      !m_ui->m_txtPassword->lineEdit()->text().isEmpty() &&
                      !m_ui->m_txtUrl->lineEdit()->text().isEmpty());
}
