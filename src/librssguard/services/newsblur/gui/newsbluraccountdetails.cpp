// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/newsblur/gui/newsbluraccountdetails.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "services/newsblur/newsblurnetwork.h"

#include <QVariantHash>

NewsBlurAccountDetails::NewsBlurAccountDetails(QWidget* parent) : QWidget(parent), m_lastProxy({}) {
  m_ui.setupUi(this);

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

  connect(m_ui.m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &NewsBlurAccountDetails::onPasswordChanged);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &NewsBlurAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &NewsBlurAccountDetails::onUrlChanged);

  setTabOrder(m_ui.m_txtUrl->lineEdit(), m_ui.m_cbDownloadOnlyUnreadMessages);
  setTabOrder(m_ui.m_cbDownloadOnlyUnreadMessages, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_txtUsername->lineEdit());
  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtPassword->lineEdit());
  setTabOrder(m_ui.m_txtPassword->lineEdit(), m_ui.m_btnTestSetup);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
}

void NewsBlurAccountDetails::performTest(const QNetworkProxy& custom_proxy) {
  m_lastProxy = custom_proxy;

  NewsBlurNetwork factory;

  factory.setUsername(m_ui.m_txtUsername->lineEdit()->text());
  factory.setPassword(m_ui.m_txtPassword->lineEdit()->text());
  factory.setBaseUrl(m_ui.m_txtUrl->lineEdit()->text());

  try {
    LoginResult result = factory.login(custom_proxy);

    if (result.m_authenticated && !result.m_sessiodId.isEmpty()) {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok, tr("You are good to go!"), tr("Yeah."));
    }
    else {
      throw ApplicationException(result.m_errors.join(QSL(", ")));
    }
  }
  catch (const NetworkException& netEx) {
    m_ui.m_lblTestResult
      ->setStatus(WidgetWithStatus::StatusType::Error,
                  tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(netEx.networkError())),
                  tr("Network error, have you entered correct username and password?"));
  }
  catch (const ApplicationException& ex) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                    tr("Error: '%1'.").arg(ex.message()),
                                    tr("Error, have you entered correct Nextcloud endpoint and password?"));
  }
}

void NewsBlurAccountDetails::onUsernameChanged() {
  const QString username = m_ui.m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void NewsBlurAccountDetails::onPasswordChanged() {
  const QString password = m_ui.m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("Password is okay."));
  }
}

void NewsBlurAccountDetails::onUrlChanged() {
  const QString url = m_ui.m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("URL cannot be empty."));
  }
  else {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is okay."));
  }
}
