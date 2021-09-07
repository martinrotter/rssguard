// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/gui/feedlyaccountdetails.h"

#include "definitions/definitions.h"
#include "exceptions/networkexception.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/systemfactory.h"
#include "network-web/webfactory.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlynetwork.h"

#if defined(FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

#include <QVariantHash>

FeedlyAccountDetails::FeedlyAccountDetails(QWidget* parent) : QWidget(parent), m_lastProxy({}) {
#if defined(FEEDLY_OFFICIAL_SUPPORT)
  m_oauth = nullptr;
#endif

  m_ui.setupUi(this);

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your account"));
  m_ui.m_txtDeveloperAccessToken->lineEdit()->setPlaceholderText(tr("Developer access token"));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test done yet."),
                                  tr("Here, results of connection test are shown."));

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  m_ui.m_lblInfo->setHelpText(tr("Your %1 build has official Feedly support. You do not have to use \"developer acess "
                                 "token\". You can therefore leave corresponding field empty.").arg(QSL(APP_NAME)),
                              false);
#else
  m_ui.m_lblInfo->setHelpText(tr("Your %1 does not offer official Feedly support, thus you must "
                                 "authorize via special authorization code called \"developer access token\". "
                                 "These tokens are usually valid only for 1 month and allow only 250 API calls "
                                 "each day.").arg(QSL(APP_NAME)),
                              true);
#endif

  m_ui.m_lblLimitMessagesInfo->setHelpText(tr("Beware of downloading too many articles, because "
                                              "Feedly permanently caches ALL articles of the feed, so you might "
                                              "end up with thousands of articles which you will never read anyway."),
                                           true);

  connect(m_ui.m_btnGetToken, &QPushButton::clicked, this, &FeedlyAccountDetails::getDeveloperAccessToken);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FeedlyAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtDeveloperAccessToken->lineEdit(), &BaseLineEdit::textChanged,
          this, &FeedlyAccountDetails::onDeveloperAccessTokenChanged);

  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_btnGetToken);
  setTabOrder(m_ui.m_btnGetToken, m_ui.m_txtDeveloperAccessToken->lineEdit());
  setTabOrder(m_ui.m_txtDeveloperAccessToken->lineEdit(), m_ui.m_checkDownloadOnlyUnreadMessages);
  setTabOrder(m_ui.m_checkDownloadOnlyUnreadMessages, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_btnTestSetup);

  onDeveloperAccessTokenChanged();
  onUsernameChanged();

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  hookNetwork();
#endif
}

void FeedlyAccountDetails::getDeveloperAccessToken() {
  qApp->web()->openUrlInExternalBrowser(FEEDLY_GENERATE_DAT);
}

#if defined(FEEDLY_OFFICIAL_SUPPORT)

void FeedlyAccountDetails::hookNetwork() {
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &FeedlyAccountDetails::onAuthGranted);
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &FeedlyAccountDetails::onAuthError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &FeedlyAccountDetails::onAuthFailed);
}

void FeedlyAccountDetails::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void FeedlyAccountDetails::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void FeedlyAccountDetails::onAuthGranted() {
  FeedlyNetwork factory;

  factory.setOauth(m_oauth);

  try {
    auto prof = factory.profile(m_lastProxy);

    m_ui.m_txtUsername->lineEdit()->setText(prof["email"].toString());
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("Tested successfully. You may be prompted to login once more."),
                                    tr("Your access was approved."));
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_FEEDLY
                << "Failed to obtain profile with error:"
                << QUOTE_W_SPACE_DOT(ex.message());
  }
}

#endif

void FeedlyAccountDetails::performTest(const QNetworkProxy& custom_proxy) {
  m_lastProxy = custom_proxy;

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  m_oauth->logout(false);

  if (m_ui.m_txtDeveloperAccessToken->lineEdit()->text().simplified().isEmpty()) {
    m_oauth->login();
    return;
  }
#endif

  FeedlyNetwork factory;

  factory.setDeveloperAccessToken(m_ui.m_txtDeveloperAccessToken->lineEdit()->text());

  try {
    auto prof = factory.profile(custom_proxy);

    m_ui.m_txtUsername->lineEdit()->setText(prof["email"].toString());
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("Login was successful."),
                                    tr("Access granted."));
  }
  catch (const NetworkException& ex) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                    tr("Error: '%1'").arg(ex.message()),
                                    tr("Some problems."));
  }
}

void FeedlyAccountDetails::onUsernameChanged() {
  const QString username = m_ui.m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void FeedlyAccountDetails::onDeveloperAccessTokenChanged() {
  const QString token = m_ui.m_txtDeveloperAccessToken->lineEdit()->text();

  if (token.isEmpty()) {
#if defined(FEEDLY_OFFICIAL_SUPPORT)
    WidgetWithStatus::StatusType stat = WidgetWithStatus::StatusType::Ok;
#else
    WidgetWithStatus::StatusType stat = WidgetWithStatus::StatusType::Error;
#endif

    m_ui.m_txtDeveloperAccessToken->setStatus(stat, tr("Access token is empty."));
  }
  else {
    m_ui.m_txtDeveloperAccessToken->setStatus(WidgetWithStatus::StatusType::Ok, tr("Access token is okay."));
  }
}
