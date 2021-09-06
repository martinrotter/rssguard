// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/gui/greaderaccountdetails.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/systemfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/greader/definitions.h"
#include "services/greader/greadernetwork.h"

#include <QVariantHash>

GreaderAccountDetails::GreaderAccountDetails(QWidget* parent) : QWidget(parent),
  m_oauth(nullptr), m_lastProxy({}) {
  m_ui.setupUi(this);

  for (auto serv : { GreaderServiceRoot::Service::Bazqux,
                     GreaderServiceRoot::Service::FreshRss,
                     GreaderServiceRoot::Service::Inoreader,
                     GreaderServiceRoot::Service::Reedah,
                     GreaderServiceRoot::Service::TheOldReader,
                     GreaderServiceRoot::Service::Other }) {
    m_ui.m_cmbService->addItem(GreaderServiceRoot::serviceToString(serv), QVariant::fromValue(serv));
  }

  m_ui.m_dateNewerThan->setMinimumDate(QDate(2000, 1, 1));
  m_ui.m_dateNewerThan->setMaximumDate(QDate::currentDate());
  m_ui.m_dateNewerThan->setDisplayFormat(qApp->localization()->loadedLocale().dateFormat());

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtPassword->lineEdit()->setPasswordMode(true);
  m_ui.m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your account"));
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your account"));
  m_ui.m_txtUrl->lineEdit()->setPlaceholderText(tr("URL of your server, without any service-specific path"));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test done yet."),
                                  tr("Here, results of connection test are shown."));

  m_ui.m_lblLimitMessages->setHelpText(tr("Some feeds might contain tens of thousands of articles "
                                          "and downloading all of them could take great amount of time, "
                                          "so sometimes it is good to download "
                                          "only certain amount of newest messages."),
                                       true);

  m_ui.m_lblNewAlgorithm->setHelpText(tr("If you select intelligent synchronization, then only not-yet-fetched "
                                         "or updated articles are downloaded. Network usage is greatly reduced and "
                                         "overall synchronization speed is greatly improved, but "
                                         "first feed fetching could be slow anyway if your feed contains "
                                         "huge number of articles."),
                                      false);

#if defined(INOREADER_OFFICIAL_SUPPORT)
  m_ui.m_lblInfo->setHelpText(tr("There are some preconfigured OAuth tokens so you do not have to fill in your "
                                 "client ID/secret, but it is strongly recommended to obtain your "
                                 "own as preconfigured tokens have limited global usage quota. If you wish "
                                 "to use preconfigured tokens, simply leave all above fields to their default values even "
                                 "if they are empty."),
                              true);
#else
  m_ui.m_lblInfo->setHelpText(tr("You have to fill in your client ID/secret and also fill in correct redirect URL."),
                              true);
#endif

  connect(m_ui.m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::onPasswordChanged);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::onUrlChanged);
  connect(m_ui.m_cmbService, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GreaderAccountDetails::fillPredefinedUrl);
  connect(m_ui.m_cbNewAlgorithm, &QCheckBox::toggled, m_ui.m_spinLimitMessages, &MessageCountSpinBox::setDisabled);
  connect(m_ui.m_txtAppId->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtAppKey->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::checkOAuthValue);
  connect(m_ui.m_txtRedirectUrl->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::checkOAuthValue);
  connect(m_ui.m_btnRegisterApi, &QPushButton::clicked, this, &GreaderAccountDetails::registerApi);

  setTabOrder(m_ui.m_cmbService, m_ui.m_txtUrl->lineEdit());
  setTabOrder(m_ui.m_txtUrl->lineEdit(), m_ui.m_cbDownloadOnlyUnreadMessages);
  setTabOrder(m_ui.m_cbDownloadOnlyUnreadMessages, m_ui.m_cbNewAlgorithm);
  setTabOrder(m_ui.m_cbNewAlgorithm, m_ui.m_dateNewerThan);
  setTabOrder(m_ui.m_dateNewerThan, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_txtUsername->lineEdit());
  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtPassword->lineEdit());
  setTabOrder(m_ui.m_txtPassword->lineEdit(), m_ui.m_txtAppId);
  setTabOrder(m_ui.m_txtAppId, m_ui.m_txtAppKey);
  setTabOrder(m_ui.m_txtAppKey, m_ui.m_txtRedirectUrl);
  setTabOrder(m_ui.m_txtRedirectUrl, m_ui.m_btnRegisterApi);
  setTabOrder(m_ui.m_btnRegisterApi, m_ui.m_btnTestSetup);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();

  emit m_ui.m_txtAppId->lineEdit()->textChanged(m_ui.m_txtAppId->lineEdit()->text());
  emit m_ui.m_txtAppKey->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());
  emit m_ui.m_txtRedirectUrl->lineEdit()->textChanged(m_ui.m_txtAppKey->lineEdit()->text());
}

void GreaderAccountDetails::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void GreaderAccountDetails::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void GreaderAccountDetails::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));

  try {
    GreaderNetwork fac;

    fac.setService(service());
    fac.setOauth(m_oauth);
    auto resp = fac.userInfo(m_lastProxy);

    m_ui.m_txtUsername->lineEdit()->setText(resp["userEmail"].toString());
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_GREADER
                << "Failed to obtain profile with error:"
                << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void GreaderAccountDetails::hookNetwork() {
  if (m_oauth != nullptr) {
    connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &GreaderAccountDetails::onAuthGranted);
    connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &GreaderAccountDetails::onAuthError);
    connect(m_oauth, &OAuth2Service::authFailed, this, &GreaderAccountDetails::onAuthFailed);
  }
}

void GreaderAccountDetails::registerApi() {
  qApp->web()->openUrlInExternalBrowser(INO_REG_API_URL);
}

void GreaderAccountDetails::checkOAuthValue(const QString& value) {
  auto* line_edit = qobject_cast<LineEditWithStatus*>(sender()->parent());

  if (line_edit != nullptr) {
    if (value.isEmpty()) {
#if defined(INOREADER_OFFICIAL_SUPPORT)
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

GreaderServiceRoot::Service GreaderAccountDetails::service() const {
  return m_ui.m_cmbService->currentData().value<GreaderServiceRoot::Service>();
}

void GreaderAccountDetails::setService(GreaderServiceRoot::Service service) {
  m_ui.m_cmbService->setCurrentIndex(m_ui.m_cmbService->findData(QVariant::fromValue(service)));
}

void GreaderAccountDetails::performTest(const QNetworkProxy& custom_proxy) {
  m_lastProxy = custom_proxy;

  if (service() == GreaderServiceRoot::Service::Inoreader) {
    if (m_oauth != nullptr) {
      m_oauth->logout(true);
      m_oauth->setClientId(m_ui.m_txtAppId->lineEdit()->text());
      m_oauth->setClientSecret(m_ui.m_txtAppKey->lineEdit()->text());
      m_oauth->setRedirectUrl(m_ui.m_txtRedirectUrl->lineEdit()->text(), true);
      m_oauth->login();
    }
  }
  else {
    GreaderNetwork factory;

    factory.setUsername(m_ui.m_txtUsername->lineEdit()->text());
    factory.setPassword(m_ui.m_txtPassword->lineEdit()->text());
    factory.setBaseUrl(m_ui.m_txtUrl->lineEdit()->text());
    factory.setService(service());
    factory.clearCredentials();

    auto result = factory.clientLogin(custom_proxy);

    if (result != QNetworkReply::NetworkError::NoError) {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                      tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(result)),
                                      tr("Network error, have you entered correct Nextcloud endpoint and password?"));
    }
    else {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                      tr("You are good to go!"),
                                      tr("Yeah."));
    }
  }
}

void GreaderAccountDetails::onUsernameChanged() {
  const QString username = m_ui.m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void GreaderAccountDetails::onPasswordChanged() {
  const QString password = m_ui.m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("Password is okay."));
  }
}

void GreaderAccountDetails::onUrlChanged() {
  const QString url = m_ui.m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("URL cannot be empty."));
  }
  else {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is okay."));
  }
}

void GreaderAccountDetails::fillPredefinedUrl() {
  switch (service()) {
    case GreaderServiceRoot::Service::Reedah:
      m_ui.m_txtUrl->lineEdit()->setText(QSL(GREADER_URL_REEDAH));
      break;

    case GreaderServiceRoot::Service::Bazqux:
      m_ui.m_txtUrl->lineEdit()->setText(QSL(GREADER_URL_BAZQUX));
      break;

    case GreaderServiceRoot::Service::TheOldReader:
      m_ui.m_txtUrl->lineEdit()->setText(QSL(GREADER_URL_TOR));
      break;

    case GreaderServiceRoot::Service::Inoreader:
      m_ui.m_txtUrl->lineEdit()->setText(QSL(GREADER_URL_INOREADER));
      break;

    default:
      m_ui.m_txtUrl->lineEdit()->clear();
      m_ui.m_txtUrl->setFocus();
      break;
  }

  // Show OAuth settings for Inoreader and classic for other services.
  m_ui.m_stackedAuth->setCurrentIndex(service() == GreaderServiceRoot::Service::Inoreader ? 1 : 0);
  m_ui.m_txtUrl->setDisabled(service() == GreaderServiceRoot::Service::Inoreader);
}
