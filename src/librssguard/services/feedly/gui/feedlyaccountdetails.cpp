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

#if defined (FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

#include <QVariantHash>

FeedlyAccountDetails::FeedlyAccountDetails(QWidget* parent) : QWidget(parent) {
#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_oauth = new OAuth2Service(QSL(FEEDLY_API_URL_BASE) + FEEDLY_API_URL_AUTH,
                              QSL(FEEDLY_API_URL_BASE) + FEEDLY_API_URL_TOKEN,
                              FEEDLY_CLIENT_ID,
                              FEEDLY_CLIENT_SECRET,
                              FEEDLY_API_SCOPE, this);

  m_oauth->setRedirectUrl(QString(OAUTH_REDIRECT_URI) + QL1C(':') + QString::number(FEEDLY_API_REDIRECT_URI_PORT));
#endif

  m_ui.setupUi(this);

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your account"));
  m_ui.m_txtDeveloperAccessToken->lineEdit()->setPlaceholderText(tr("Developer access token"));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test done yet."),
                                  tr("Here, results of connection test are shown."));

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_ui.m_lblInfo->setText(tr("Your %1 build has official Feedly support. You do not have to use \"developer acess "
                             "token\". You can therefore leave corresponding field empty.").arg(APP_NAME));
#else
  m_ui.m_lblInfo->setText(tr("Your %1 does not offer official Feedly support, thus you must "
                             "authorize via special authorization code called \"developer access token\". "
                             "These tokens are usually valid only for 1 month and allow only 250 API calls "
                             "each day.").arg(APP_NAME));
#endif

  m_ui.m_lblLimitMessagesInfo->setText(tr("Be very careful about downloading too many messages, because "
                                          "Feedly automagically caches ALL messages of a feed forever so you might "
                                          "end with thousands of messages you will never read anyway."));

  connect(m_ui.m_spinLimitMessages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("= unlimited"));
    }
    else {
      m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("messages"));
    }
  });

  GuiUtilities::setLabelAsNotice(*m_ui.m_lblInfo, true);
  GuiUtilities::setLabelAsNotice(*m_ui.m_lblLimitMessagesInfo, true);

  connect(m_ui.m_btnGetToken, &QPushButton::clicked, this, &FeedlyAccountDetails::getDeveloperAccessToken);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FeedlyAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtDeveloperAccessToken->lineEdit(), &BaseLineEdit::textChanged,
          this, &FeedlyAccountDetails::onDeveloperAccessTokenChanged);

  m_ui.m_spinLimitMessages->setMinimum(FEEDLY_UNLIMITED_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMaximum(FEEDLY_MAX_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setValue(FEEDLY_DEFAULT_BATCH_SIZE);

  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_btnGetToken);
  setTabOrder(m_ui.m_btnGetToken, m_ui.m_txtDeveloperAccessToken->lineEdit());
  setTabOrder(m_ui.m_txtDeveloperAccessToken->lineEdit(), m_ui.m_checkDownloadOnlyUnreadMessages);
  setTabOrder(m_ui.m_checkDownloadOnlyUnreadMessages, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_btnTestSetup);

  onDeveloperAccessTokenChanged();
  onUsernameChanged();

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  hookNetwork();
#endif
}

void FeedlyAccountDetails::getDeveloperAccessToken() {
  qApp->web()->openUrlInExternalBrowser(FEEDLY_GENERATE_DAT);
}

#if defined (FEEDLY_OFFICIAL_SUPPORT)

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
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));
}

#endif

void FeedlyAccountDetails::performTest(const QNetworkProxy& custom_proxy) {
#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_oauth->logout(false);

  if (m_ui.m_txtDeveloperAccessToken->lineEdit()->text().simplified().isEmpty()) {
    if (m_oauth->login()) {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                      tr("You are already logged in."),
                                      tr("Access granted."));
    }

    return;
  }
#endif

  FeedlyNetwork factory;

  factory.setUsername(m_ui.m_txtUsername->lineEdit()->text());
  factory.setDeveloperAccessToken(m_ui.m_txtDeveloperAccessToken->lineEdit()->text());

  try {
    m_ui.m_txtUsername->lineEdit()->setText(factory.profile(custom_proxy)["email"].toString());
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
#if defined (FEEDLY_OFFICIAL_SUPPORT)
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
