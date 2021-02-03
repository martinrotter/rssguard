// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/gui/greaderaccountdetails.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "miscellaneous/systemfactory.h"
#include "services/greader/definitions.h"
#include "services/greader/greadernetwork.h"

GreaderAccountDetails::GreaderAccountDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  for (auto serv : { GreaderServiceRoot::Service::Bazqux,
                     GreaderServiceRoot::Service::FreshRss,
                     GreaderServiceRoot::Service::Reedah,
                     GreaderServiceRoot::Service::TheOldReader,
                     GreaderServiceRoot::Service::Other }) {
    m_ui.m_cmbService->addItem(GreaderNetwork::serviceToString(serv), QVariant::fromValue(serv));
  }

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your account"));
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your account"));
  m_ui.m_txtUrl->lineEdit()->setPlaceholderText(tr("URL of your server, without any service-specific path"));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test done yet."),
                                  tr("Here, results of connection test are shown."));
  m_ui.m_lblLimitMessages->setText(tr("Limiting number of downloaded messages per feed makes updating of "
                                      "feeds faster, but if your feed contains bigger number of messages "
                                      "than specified limit, then some older messages might not be "
                                      "downloaded during feed update."));

  connect(m_ui.m_spinLimitMessages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("= unlimited"));
    }
    else {
      m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("messages"));
    }
  });

  GuiUtilities::setLabelAsNotice(*m_ui.m_lblLimitMessages, true);

  connect(m_ui.m_checkShowPassword, &QCheckBox::toggled, this, &GreaderAccountDetails::displayPassword);
  connect(m_ui.m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::onPasswordChanged);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &GreaderAccountDetails::onUrlChanged);

  setTabOrder(m_ui.m_cmbService, m_ui.m_txtUrl->lineEdit());
  setTabOrder(m_ui.m_txtUrl->lineEdit(), m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_txtUsername->lineEdit());
  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtPassword->lineEdit());
  setTabOrder(m_ui.m_txtPassword->lineEdit(), m_ui.m_checkShowPassword);
  setTabOrder(m_ui.m_checkShowPassword, m_ui.m_btnTestSetup);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
  displayPassword(false);
}

GreaderServiceRoot::Service GreaderAccountDetails::service() const {
  return m_ui.m_cmbService->currentData().value<GreaderServiceRoot::Service>();
}

void GreaderAccountDetails::setService(GreaderServiceRoot::Service service) {
  m_ui.m_cmbService->setCurrentIndex(m_ui.m_cmbService->findData(QVariant::fromValue(service)));
}

void GreaderAccountDetails::displayPassword(bool display) {
  m_ui.m_txtPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void GreaderAccountDetails::performTest(const QNetworkProxy& custom_proxy) {
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
