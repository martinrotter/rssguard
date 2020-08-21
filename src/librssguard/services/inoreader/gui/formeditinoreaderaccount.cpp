// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/inoreader/gui/formeditinoreaderaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/inoreaderserviceroot.h"

FormEditInoreaderAccount::FormEditInoreaderAccount(QWidget* parent)
  : QDialog(parent), m_oauth(nullptr), m_editableRoot(nullptr) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("inoreader")));
  GuiUtilities::setLabelAsNotice(*m_ui.m_lblInfo, true);

  m_ui.m_lblInfo->setText(tr("Specified redirect URL must start with \"http://localhost\" and "
                             "must be configured in your OAuth \"application\".\n\n"
                             "It is highly recommended to create your own \"Application ID\"."));

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

  connect(m_ui.m_txtAppId->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkOAuthValue);
  connect(m_ui.m_txtAppKey->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkOAuthValue);
  connect(m_ui.m_txtRedirectUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkOAuthValue);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkUsername);
  connect(m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditInoreaderAccount::testSetup);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormEditInoreaderAccount::onClickedOk);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormEditInoreaderAccount::onClickedCancel);
  connect(m_ui.m_btnRegisterApi, &QPushButton::clicked, this, &FormEditInoreaderAccount::registerApi);

  m_ui.m_spinLimitMessages->setValue(INOREADER_DEFAULT_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMinimum(INOREADER_MIN_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMaximum(INOREADER_MAX_BATCH_SIZE);

  checkUsername(m_ui.m_txtUsername->lineEdit()->text());
}

FormEditInoreaderAccount::~FormEditInoreaderAccount() = default;

void FormEditInoreaderAccount::testSetup() {
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

void FormEditInoreaderAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new InoreaderServiceRoot(nullptr);
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

void FormEditInoreaderAccount::onClickedCancel() {
  reject();
}

void FormEditInoreaderAccount::checkUsername(const QString& username) {
  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("No username entered."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some username entered."));
  }
}

void FormEditInoreaderAccount::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void FormEditInoreaderAccount::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void FormEditInoreaderAccount::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));
}

void FormEditInoreaderAccount::hookNetwork() {
  connect(m_oauth, &OAuth2Service::tokensReceived, this, &FormEditInoreaderAccount::onAuthGranted);
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &FormEditInoreaderAccount::onAuthError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &FormEditInoreaderAccount::onAuthFailed);
}

InoreaderServiceRoot* FormEditInoreaderAccount::execForCreate() {
  setWindowTitle(tr("Add new Inoreader account"));

  m_oauth = new OAuth2Service(INOREADER_OAUTH_AUTH_URL, INOREADER_OAUTH_TOKEN_URL,
                              INOREADER_OAUTH_CLI_ID, INOREADER_OAUTH_CLI_KEY, INOREADER_OAUTH_SCOPE, this);
  hookNetwork();

  m_ui.m_txtAppId->lineEdit()->setText(INOREADER_OAUTH_CLI_ID);
  m_ui.m_txtAppKey->lineEdit()->setText(INOREADER_OAUTH_CLI_KEY);
  m_ui.m_txtRedirectUrl->lineEdit()->setText(OAUTH_REDIRECT_URI);

  exec();

  return m_editableRoot;
}

void FormEditInoreaderAccount::execForEdit(InoreaderServiceRoot* existing_root) {
  setWindowTitle(tr("Edit existing Inoreader account"));
  m_editableRoot = existing_root;

  m_oauth = m_editableRoot->network()->oauth();
  hookNetwork();

  // Setup the GUI.
  m_ui.m_txtAppId->lineEdit()->setText(m_oauth->clientId());
  m_ui.m_txtAppKey->lineEdit()->setText(m_oauth->clientSecret());
  m_ui.m_txtRedirectUrl->lineEdit()->setText(m_oauth->redirectUrl());

  m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->userName());
  m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());

  exec();
}

void FormEditInoreaderAccount::registerApi() {
  qApp->web()->openUrlInExternalBrowser(INOREADER_REG_API_URL);
}

void FormEditInoreaderAccount::checkOAuthValue(const QString& value) {
  auto* line_edit = qobject_cast<LineEditWithStatus*>(sender()->parent());

  if (line_edit != nullptr) {
    if (value.isEmpty()) {
      line_edit->setStatus(WidgetWithStatus::StatusType::Error, tr("Empty value is entered."));
    }
    else {
      line_edit->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some value is entered."));
    }
  }
}
