// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/gmailaccountdetails.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/gmail/definitions.h"
#include "services/gmail/network/gmailnetworkfactory.h"

GmailAccountDetails::GmailAccountDetails(QWidget* parent)
  : QWidget(parent), m_oauth(new OAuth2Service(GMAIL_OAUTH_AUTH_URL, GMAIL_OAUTH_TOKEN_URL,
                                               {}, {}, GMAIL_OAUTH_SCOPE, this)) {
  m_ui.setupUi(this);

  GuiUtilities::setLabelAsNotice(*m_ui.m_lblInfo, true);

  m_ui.m_lblInfo->setText(tr("Specified redirect URL must start with \"http://localhost\" and "
                             "must be configured in your OAuth \"application\"."));

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

  connect(m_ui.m_txtAppId->lineEdit(), &BaseLineEdit::textChanged, this, &GmailAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtAppKey->lineEdit(), &BaseLineEdit::textChanged, this, &GmailAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtRedirectUrl->lineEdit(), &BaseLineEdit::textChanged, this, &GmailAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &GmailAccountDetails::checkUsername);
  connect(m_ui.m_btnTestSetup, &QPushButton::clicked, this, &GmailAccountDetails::testSetup);
  connect(m_ui.m_btnRegisterApi, &QPushButton::clicked, this, &GmailAccountDetails::registerApi);

  m_ui.m_spinLimitMessages->setValue(GMAIL_DEFAULT_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMinimum(GMAIL_MIN_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMaximum(GMAIL_MAX_BATCH_SIZE);

  emit m_ui.m_txtUsername->lineEdit()->textChanged(m_ui.m_txtUsername->lineEdit()->text());
  emit m_ui.m_txtAppId->lineEdit()->textChanged(m_ui.m_txtAppId->lineEdit()->text());
  emit m_ui.m_txtAppKey->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());

  m_ui.m_txtRedirectUrl->lineEdit()->setText(QString(OAUTH_REDIRECT_URI) +
                                             QL1C(':') +
                                             QString::number(OAUTH_REDIRECT_URI_PORT));

  hookNetwork();
}

void GmailAccountDetails::testSetup() {
  m_oauth->logout();
  m_oauth->setClientId(m_ui.m_txtAppId->lineEdit()->text());
  m_oauth->setClientSecret(m_ui.m_txtAppKey->lineEdit()->text());
  m_oauth->setRedirectUrl(m_ui.m_txtRedirectUrl->lineEdit()->text());

  if (m_oauth->login()) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("You are already logged in."),
                                    tr("Access granted."));
  }
}

void GmailAccountDetails::checkUsername(const QString& username) {
  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("No username entered."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some username entered."));
  }
}

void GmailAccountDetails::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void GmailAccountDetails::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1 ").arg(detailed_description),
                                  tr("There was error during testing."));
}

void GmailAccountDetails::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));
}

void GmailAccountDetails::hookNetwork() {
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &GmailAccountDetails::onAuthGranted);
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &GmailAccountDetails::onAuthError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &GmailAccountDetails::onAuthFailed);
}

void GmailAccountDetails::registerApi() {
  qApp->web()->openUrlInExternalBrowser(GMAIL_REG_API_URL);
}

void GmailAccountDetails::checkOAuthValue(const QString& value) {
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
