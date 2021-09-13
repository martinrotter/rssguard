// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/gmailaccountdetails.h"

#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailnetworkfactory.h"

GmailAccountDetails::GmailAccountDetails(QWidget* parent)
  : QWidget(parent), m_oauth(nullptr), m_lastProxy({}) {
  m_ui.setupUi(this);

#if defined(GMAIL_OFFICIAL_SUPPORT)
  m_ui.m_lblInfo->setHelpText(tr("There are some preconfigured OAuth tokens so you do not have to fill in your "
                                 "client ID/secret, but it is strongly recommended to obtain your "
                                 "own as it preconfigured tokens have limited global usage quota. If you wish "
                                 "to use preconfigured tokens, simply leave those fields empty and make sure "
                                 "to leave default value of redirect URL."),
                              false);
#else
  m_ui.m_lblInfo->setHelpText(tr("You have to fill in your client ID/secret and also fill in correct redirect URL."),
                              true);
#endif

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
  connect(m_ui.m_btnRegisterApi, &QPushButton::clicked, this, &GmailAccountDetails::registerApi);

  emit m_ui.m_txtUsername->lineEdit()->textChanged(m_ui.m_txtUsername->lineEdit()->text());
  emit m_ui.m_txtAppId->lineEdit()->textChanged(m_ui.m_txtAppId->lineEdit()->text());
  emit m_ui.m_txtAppKey->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());
  emit m_ui.m_txtRedirectUrl->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());

  hookNetwork();
}

void GmailAccountDetails::testSetup(const QNetworkProxy& custom_proxy) {
  m_oauth->logout(true);
  m_oauth->setClientId(m_ui.m_txtAppId->lineEdit()->text());
  m_oauth->setClientSecret(m_ui.m_txtAppKey->lineEdit()->text());
  m_oauth->setRedirectUrl(m_ui.m_txtRedirectUrl->lineEdit()->text(), true);

  m_lastProxy = custom_proxy;
  m_oauth->login();
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
                                  tr("There is error: %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void GmailAccountDetails::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));

  try {
    GmailNetworkFactory fac;

    fac.setOauth(m_oauth);

    auto resp = fac.getProfile(m_lastProxy);

    m_ui.m_txtUsername->lineEdit()->setText(resp[QSL("emailAddress")].toString());
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_GMAIL
                << "Failed to obtain profile with error:"
                << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void GmailAccountDetails::hookNetwork() {
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &GmailAccountDetails::onAuthGranted);
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &GmailAccountDetails::onAuthError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &GmailAccountDetails::onAuthFailed);
}

void GmailAccountDetails::registerApi() {
  qApp->web()->openUrlInExternalBrowser(QSL(GMAIL_REG_API_URL));
}

void GmailAccountDetails::checkOAuthValue(const QString& value) {
  auto* line_edit = qobject_cast<LineEditWithStatus*>(sender()->parent());

  if (line_edit != nullptr) {
    if (value.isEmpty()) {
#if defined(GMAIL_OFFICIAL_SUPPORT)
      line_edit->setStatus(WidgetWithStatus::StatusType::Ok, tr("Preconfigured client ID/secret will be used."));
#else
      line_edit->setStatus(WidgetWithStatus::StatusType::Error, tr("Empty value is entered."));
#endif
    }
    else {
      line_edit->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some value is entered."));
    }
  }
}
