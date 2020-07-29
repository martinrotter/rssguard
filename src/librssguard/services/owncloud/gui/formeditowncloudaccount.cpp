// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/gui/formeditowncloudaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/owncloud/definitions.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/owncloudserviceroot.h"

FormEditOwnCloudAccount::FormEditOwnCloudAccount(QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormEditOwnCloudAccount), m_editableRoot(nullptr) {
  m_ui->setupUi(this);
  m_btnOk = m_ui->m_buttonBox->button(QDialogButtonBox::Ok);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("nextcloud")));

  m_ui->m_lblTestResult->label()->setWordWrap(true);
  m_ui->m_lblServerSideUpdateInformation->setText(tr("Leaving this option on causes that updates "
                                                     "of feeds will be probably much slower and may time-out often."));
  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your Nextcloud account"));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your Nextcloud account"));
  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("URL of your Nextcloud server, without any API path"));
  m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                   tr("No test done yet."),
                                   tr("Here, results of connection test are shown."));
  m_ui->m_lblLimitMessages->setText(
    tr("Limiting number of downloaded messages per feed makes updating of feeds faster but if your feed contains "
       "bigger number of messages than specified limit, then some messages might not be downloaded during feed update."));

  connect(m_ui->m_spinLimitMessages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui->m_spinLimitMessages->setSuffix(QSL(" ") + tr("= unlimited"));
    }
    else {
      m_ui->m_spinLimitMessages->setSuffix(QSL(" ") + tr("messages"));
    }
  });

  GuiUtilities::setLabelAsNotice(*m_ui->m_lblLimitMessages, false);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblServerSideUpdateInformation, false);

  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_checkServerSideUpdate);
  setTabOrder(m_ui->m_checkServerSideUpdate, m_ui->m_spinLimitMessages);
  setTabOrder(m_ui->m_spinLimitMessages, m_ui->m_txtUsername->lineEdit());
  setTabOrder(m_ui->m_txtUsername->lineEdit(), m_ui->m_txtPassword->lineEdit());
  setTabOrder(m_ui->m_txtPassword->lineEdit(), m_ui->m_checkShowPassword);
  setTabOrder(m_ui->m_checkShowPassword, m_ui->m_btnTestSetup);
  setTabOrder(m_ui->m_btnTestSetup, m_ui->m_buttonBox);

  connect(m_ui->m_checkShowPassword, &QCheckBox::toggled, this, &FormEditOwnCloudAccount::displayPassword);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormEditOwnCloudAccount::onClickedOk);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &FormEditOwnCloudAccount::onClickedCancel);
  connect(m_ui->m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditOwnCloudAccount::onPasswordChanged);
  connect(m_ui->m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditOwnCloudAccount::onUsernameChanged);
  connect(m_ui->m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditOwnCloudAccount::onUrlChanged);
  connect(m_ui->m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditOwnCloudAccount::checkOkButton);
  connect(m_ui->m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditOwnCloudAccount::checkOkButton);
  connect(m_ui->m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditOwnCloudAccount::checkOkButton);
  connect(m_ui->m_btnTestSetup, &QPushButton::clicked, this, &FormEditOwnCloudAccount::performTest);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
  checkOkButton();
  displayPassword(false);
}

FormEditOwnCloudAccount::~FormEditOwnCloudAccount() = default;

OwnCloudServiceRoot* FormEditOwnCloudAccount::execForCreate() {
  setWindowTitle(tr("Add new Nextcloud News account"));
  exec();
  return m_editableRoot;
}

void FormEditOwnCloudAccount::execForEdit(OwnCloudServiceRoot* existing_root) {
  setWindowTitle(tr("Edit existing Nextcloud News account"));

  m_editableRoot = existing_root;
  m_ui->m_txtUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_ui->m_txtPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_ui->m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_ui->m_checkDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
  m_ui->m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());
  m_ui->m_spinLimitMessages->setValue(existing_root->network()->batchSize());

  exec();
}

void FormEditOwnCloudAccount::displayPassword(bool display) {
  m_ui->m_txtPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void FormEditOwnCloudAccount::performTest() {
  OwnCloudNetworkFactory factory;

  factory.setAuthUsername(m_ui->m_txtUsername->lineEdit()->text());
  factory.setAuthPassword(m_ui->m_txtPassword->lineEdit()->text());
  factory.setUrl(m_ui->m_txtUrl->lineEdit()->text());
  factory.setForceServerSideUpdate(m_ui->m_checkServerSideUpdate->isChecked());

  OwnCloudStatusResponse result = factory.status();

  if (result.isLoaded()) {
    if (!SystemFactory::isVersionEqualOrNewer(result.version(), OWNCLOUD_MIN_VERSION)) {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr(
                                         "Selected Nextcloud News server is running unsupported version %1. At least version %2 is required.").arg(
                                         result.version(),
                                         OWNCLOUD_MIN_VERSION),
                                       tr("Selected Nextcloud News server is running unsupported version."));
    }
    else {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr(
                                         "Nextcloud News server is okay, running with version %1, while at least version %2 is required.").arg(
                                         result.version(),
                                         OWNCLOUD_MIN_VERSION),
                                       tr("Nextcloud News server is okay."));
    }
  }
  else if (factory.lastError()  != QNetworkReply::NoError) {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                     tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(factory.lastError())),
                                     tr("Network error, have you entered correct Nextcloud endpoint and password?"));
  }
  else {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                     tr("Unspecified error, did you enter correct URL?"),
                                     tr("Unspecified error, did you enter correct URL?"));
  }
}

void FormEditOwnCloudAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new OwnCloudServiceRoot();
    editing_account = false;
  }

  m_editableRoot->network()->setUrl(m_ui->m_txtUrl->lineEdit()->text());
  m_editableRoot->network()->setAuthUsername(m_ui->m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setAuthPassword(m_ui->m_txtPassword->lineEdit()->text());
  m_editableRoot->network()->setForceServerSideUpdate(m_ui->m_checkServerSideUpdate->isChecked());
  m_editableRoot->network()->setBatchSize(m_ui->m_spinLimitMessages->value());
  m_editableRoot->network()->setDownloadOnlyUnreadMessages(m_ui->m_checkDownloadOnlyUnreadMessages->isChecked());

  m_editableRoot->saveAccountDataToDatabase();
  accept();

  if (editing_account) {
    m_editableRoot->completelyRemoveAllData();
    m_editableRoot->syncIn();
  }
}

void FormEditOwnCloudAccount::onClickedCancel() {
  reject();
}

void FormEditOwnCloudAccount::onUsernameChanged() {
  const QString username = m_ui->m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void FormEditOwnCloudAccount::onPasswordChanged() {
  const QString password = m_ui->m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::StatusType::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("Password is okay."));
  }
}

void FormEditOwnCloudAccount::onUrlChanged() {
  const QString url = m_ui->m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("URL cannot be empty."));
  }
  else {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is okay."));
  }
}

void FormEditOwnCloudAccount::checkOkButton() {
  m_btnOk->setEnabled(!m_ui->m_txtUsername->lineEdit()->text().isEmpty() &&
                      !m_ui->m_txtPassword->lineEdit()->text().isEmpty() &&
                      !m_ui->m_txtUrl->lineEdit()->text().isEmpty());
}
