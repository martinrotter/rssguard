// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/reddit/gui/redditaccountdetails.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/reddit/definitions.h"
#include "services/reddit/redditnetworkfactory.h"

RedditAccountDetails::RedditAccountDetails(QWidget* parent) : QWidget(parent), m_oauth(nullptr), m_lastProxy({}) {
  m_ui.setupUi(this);

  m_ui.m_lblInfo->setHelpText(tr("You have to fill in your client ID/secret and also fill in correct redirect URL."),
                              true);
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

  connect(m_ui.m_txtAppId->lineEdit(), &BaseLineEdit::textChanged, this, &RedditAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtAppKey->lineEdit(), &BaseLineEdit::textChanged, this, &RedditAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtRedirectUrl->lineEdit(), &BaseLineEdit::textChanged, this, &RedditAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &RedditAccountDetails::checkUsername);
  connect(m_ui.m_btnRegisterApi, &QPushButton::clicked, this, &RedditAccountDetails::registerApi);

  emit m_ui.m_txtUsername->lineEdit()->textChanged(m_ui.m_txtUsername->lineEdit()->text());
  emit m_ui.m_txtAppId->lineEdit()->textChanged(m_ui.m_txtAppId->lineEdit()->text());
  emit m_ui.m_txtAppKey->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());
  emit m_ui.m_txtRedirectUrl->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());

  hookNetwork();
}

void RedditAccountDetails::testSetup(const QNetworkProxy& custom_proxy) {
  m_oauth->logout(true);
  m_oauth->setClientId(m_ui.m_txtAppId->lineEdit()->text());
  m_oauth->setClientSecret(m_ui.m_txtAppKey->lineEdit()->text());
  m_oauth->setRedirectUrl(m_ui.m_txtRedirectUrl->lineEdit()->text(), true);

  m_lastProxy = custom_proxy;
  m_oauth->login();
}

void RedditAccountDetails::checkUsername(const QString& username) {
  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("No username entered."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some username entered."));
  }
}

void RedditAccountDetails::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void RedditAccountDetails::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error: %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void RedditAccountDetails::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));

  try {
    RedditNetworkFactory fac;

    fac.setOauth(m_oauth);

    auto resp = fac.me(m_lastProxy);

    m_ui.m_txtUsername->lineEdit()->setText(resp[QSL("name")].toString());
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_REDDIT << "Failed to obtain profile with error:" << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void RedditAccountDetails::hookNetwork() {
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &RedditAccountDetails::onAuthGranted);
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &RedditAccountDetails::onAuthError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &RedditAccountDetails::onAuthFailed);
}

void RedditAccountDetails::registerApi() {
  qApp->web()->openUrlInExternalBrowser(QSL(REDDIT_REG_API_URL));
}

void RedditAccountDetails::checkOAuthValue(const QString& value) {
  auto* line_edit = qobject_cast<LineEditWithStatus*>(sender()->parent());

  if (line_edit != nullptr) {
    if (value.isEmpty()) {
#if defined(REDDIT_OFFICIAL_SUPPORT)
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
