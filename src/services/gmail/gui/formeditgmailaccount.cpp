// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/formeditgmailaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailserviceroot.h"

FormEditGmailAccount::FormEditGmailAccount(QWidget* parent) : QDialog(parent),
  m_oauth(new OAuth2Service(GMAIL_OAUTH_AUTH_URL, GMAIL_OAUTH_TOKEN_URL,
                            QString(), QString(), GMAIL_OAUTH_SCOPE)), m_editableRoot(nullptr) {
  m_ui.setupUi(this);

  GuiUtilities::setLabelAsNotice(*m_ui.m_lblAuthInfo, true);

#if !defined(USE_WEBENGINE)
  m_ui.m_lblAuthInfo->setText(tr("You must use \"%1\" as base redirect URL. You can use custom port to make sure "
                                 "that no local service occupies it. Make sure that this redirect URL matches redirect "
                                 "URL of used \"application\".").arg(LOCALHOST_ADDRESS));
#endif

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("gmail")));

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("Not tested yet."),
                                  tr("Not tested yet."));
  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("User-visible username"));

  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtAppId);
  setTabOrder(m_ui.m_txtAppId, m_ui.m_txtAppKey);
  setTabOrder(m_ui.m_txtAppKey, m_ui.m_txtRedirectUrl);
  setTabOrder(m_ui.m_txtRedirectUrl, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_btnTestSetup);
  setTabOrder(m_ui.m_btnTestSetup, m_ui.m_buttonBox);

  connect(m_ui.m_txtAppId->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditGmailAccount::checkOAuthValue);
  connect(m_ui.m_txtAppKey->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditGmailAccount::checkOAuthValue);
  connect(m_ui.m_txtRedirectUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditGmailAccount::checkOAuthValue);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditGmailAccount::checkUsername);
  connect(m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditGmailAccount::testSetup);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormEditGmailAccount::onClickedOk);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormEditGmailAccount::onClickedCancel);

  m_ui.m_spinLimitMessages->setValue(GMAIL_DEFAULT_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMinimum(GMAIL_MIN_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMaximum(GMAIL_MAX_BATCH_SIZE);

  checkUsername(m_ui.m_txtUsername->lineEdit()->text());
  hookNetwork();
}

FormEditGmailAccount::~FormEditGmailAccount() = default;

void FormEditGmailAccount::testSetup() {
  if (m_oauth->clientId() != m_ui.m_txtAppId->lineEdit()->text() ||
      m_oauth->clientSecret() != m_ui.m_txtAppKey->lineEdit()->text() ||
      m_oauth->redirectUrl() != m_ui.m_txtRedirectUrl->lineEdit()->text()) {
    // User changed some important settings. Log out.
    m_oauth->logout();
  }

  m_oauth->setClientId(m_ui.m_txtAppId->lineEdit()->text());
  m_oauth->setClientSecret(m_ui.m_txtAppKey->lineEdit()->text());
  m_oauth->setRedirectUrl(m_ui.m_txtRedirectUrl->lineEdit()->text());

  if (m_oauth->login()) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("You are already logged in."),
                                    tr("Access granted."));
  }
}

void FormEditGmailAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new GmailServiceRoot(nullptr);
    editing_account = false;
  }

  // We copy credentials from testing OAuth to live OAuth.
  m_editableRoot->network()->oauth()->setAccessToken(m_oauth->accessToken());
  m_editableRoot->network()->oauth()->setRefreshToken(m_oauth->refreshToken());
  m_editableRoot->network()->oauth()->setTokensExpireIn(m_oauth->tokensExpireIn());

  m_editableRoot->network()->oauth()->setClientId(m_ui.m_txtAppId->lineEdit()->text());
  m_editableRoot->network()->oauth()->setClientSecret(m_ui.m_txtAppKey->lineEdit()->text());
  m_editableRoot->network()->oauth()->setRedirectUrl(m_ui.m_txtRedirectUrl->lineEdit()->text());

  m_editableRoot->network()->setUsername(m_ui.m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setBatchSize(m_ui.m_spinLimitMessages->value());
  m_editableRoot->saveAccountDataToDatabase();
  accept();

  if (editing_account) {
    m_editableRoot->completelyRemoveAllData();
    m_editableRoot->syncIn();
  }
}

void FormEditGmailAccount::onClickedCancel() {
  reject();
}

void FormEditGmailAccount::checkUsername(const QString& username) {
  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("No username entered. \
  "));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some username entered. \
  "));
  }
}

void FormEditGmailAccount::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void FormEditGmailAccount::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1 ").arg(detailed_description),
                                  tr("There was error during testing."));
}

void FormEditGmailAccount::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));
}

void FormEditGmailAccount::hookNetwork() {
  connect(m_oauth, &OAuth2Service::tokensReceived, this, &FormEditGmailAccount::onAuthGranted);
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &FormEditGmailAccount::onAuthError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &FormEditGmailAccount::onAuthFailed);
}

GmailServiceRoot* FormEditGmailAccount::execForCreate() {
  setWindowTitle(tr("Add new Gmail account"));

  m_ui.m_txtAppId->lineEdit()->clear();
  m_ui.m_txtAppKey->lineEdit()->clear();
  m_ui.m_txtRedirectUrl->lineEdit()->setText(LOCALHOST_ADDRESS);

  exec();

  return m_editableRoot;
}

void FormEditGmailAccount::execForEdit(GmailServiceRoot* existing_root) {
  setWindowTitle(tr("Edit existing Gmail account"));
  m_editableRoot = existing_root;

  // We copy settings from existing OAuth to our testing OAuth.
  m_oauth->setClientId(existing_root->network()->oauth()->clientId());
  m_oauth->setClientSecret(existing_root->network()->oauth()->clientSecret());
  m_oauth->setRedirectUrl(existing_root->network()->oauth()->redirectUrl());
  m_oauth->setRefreshToken(existing_root->network()->oauth()->refreshToken());
  m_oauth->setAccessToken(existing_root->network()->oauth()->accessToken());
  m_oauth->setTokensExpireIn(existing_root->network()->oauth()->tokensExpireIn());

  // Setup the GUI.
  m_ui.m_txtAppId->lineEdit()->setText(existing_root->network()->oauth()->clientId());
  m_ui.m_txtAppKey->lineEdit()->setText(existing_root->network()->oauth()->clientSecret());
  m_ui.m_txtRedirectUrl->lineEdit()->setText(existing_root->network()->oauth()->redirectUrl());

  m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->userName());
  m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());

  exec();
}

void FormEditGmailAccount::checkOAuthValue(const QString& value) {
  auto* line_edit = qobject_cast<LineEditWithStatus*>(sender()->parent());

  if (line_edit != nullptr) {
    if (value.isEmpty()) {
      line_edit->setStatus(WidgetWithStatus::Error, tr("Empty value is entered."));
    }
    else {
      line_edit->setStatus(WidgetWithStatus::Ok, tr("Some value is entered."));
    }
  }
}
