// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/xmppaccountdetails.h"

#include "src/definitions.h"
#include "src/xmppnetwork.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/networkfactory.h>

#include <QFileDevice>
#include <QXmppAuthenticationError.h>
#include <QXmppClient.h>
#include <QXmppUtils.h>

XmppAccountDetails::XmppAccountDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your TT-RSS account"));
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your TT-RSS account"));
  m_ui.m_txtHost->lineEdit()->setPlaceholderText(tr("URL of your XMPP server"));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test done yet."),
                                  tr("Here, results of connection test are shown."));

  setTabOrder(m_ui.m_txtHost->lineEdit(), m_ui.m_txtAdditionalServices);
  setTabOrder(m_ui.m_txtAdditionalServices, m_ui.m_txtUsername->lineEdit());
  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtPassword->lineEdit());

  m_ui.m_txtPassword->lineEdit()->setPasswordMode(true);

  connect(m_ui.m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &XmppAccountDetails::onPasswordChanged);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &XmppAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textEdited, this, &XmppAccountDetails::onUsernameEdited);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
}

void XmppAccountDetails::performTest(const QNetworkProxy& proxy) {
  XmppNetwork* factory = new XmppNetwork();

  factory->setUsername(m_ui.m_txtUsername->lineEdit()->text());
  factory->setPassword(m_ui.m_txtPassword->lineEdit()->text());
  factory->setDomain(m_ui.m_txtHost->lineEdit()->text());

  auto configuration = factory->xmppConfiguration();

  if (proxy.type() != QNetworkProxy::ProxyType::DefaultProxy) {
    configuration.setNetworkProxy(proxy);
  }

  factory->xmppClient()->connectToServer(configuration);

  connect(factory->xmppClient(), &QXmppClient::connected, this, [=, this]() {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok, tr("You are good to go!"), tr("Yeah."));
    factory->deleteLater();
  });

  connect(factory->xmppClient(), &QXmppClient::errorOccurred, this, [=, this](const QXmppError& error) {
    if (error.isNetworkError()) {
      std::optional<QNetworkReply::NetworkError> net_err = error.value<QNetworkReply::NetworkError>();
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                      tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(*net_err)),
                                      tr("Network error, have you entered correct host, username and password?"));
    }
    else if (error.isFileError()) {
      std::optional<QFileDevice::FileError> fil_err = error.value<QFileDevice::FileError>();
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                      tr("File error: '%1'.").arg(QString::number(*fil_err)),
                                      tr("File error"));
    }
    else if (error.isStanzaError()) {
      std::optional<QXmppStanza::Error> sta_err = error.value<QXmppStanza::Error>();
      m_ui.m_lblTestResult
        ->setStatus(WidgetWithStatus::StatusType::Error,
                    tr("Client error: '%1 - %2'.").arg(QString::number((*sta_err).type()), (*sta_err).text()),
                    tr("Client error"));
    }
    else {
      std::optional<QXmpp::AuthenticationError> auth_err = error.value<QXmpp::AuthenticationError>();

      if (auth_err.has_value()) {
        m_ui.m_lblTestResult
          ->setStatus(WidgetWithStatus::StatusType::Error,
                      tr("Auth error: '%1 - %2'.").arg(QString::number((*auth_err).type), (*auth_err).text),
                      tr("Auth error"));
      }
      else {
        m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                        tr("Generic error: '%1'.").arg(error.description),
                                        tr("Generic error"));
      }
    }

    factory->deleteLater();
  });

  /*
  factory.setUrl(m_ui.m_txtUrl->lineEdit()->text());
  factory.setAuthIsUsed(m_ui.m_gbHttpAuthentication->isChecked());
  factory.setAuthUsername(m_ui.m_txtHttpUsername->lineEdit()->text());
  factory.setAuthPassword(m_ui.m_txtHttpPassword->lineEdit()->text());
  factory.setForceServerSideUpdate(m_ui.m_checkServerSideUpdate->isChecked());
  factory.setBatchSize(m_ui.m_spinLimitMessages->value());

  XmppLoginResponse result = factory.login(proxy);

  if (result.isLoaded()) {
    if (result.hasError()) {
      QString error = result.error();

      if (error == QSL(XMPP_API_DISABLED)) {
        m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                        tr("API access on selected server is not enabled."),
                                        tr("API access on selected server is not enabled."));
      }
      else if (error == QSL(XMPP_LOGIN_ERROR)) {
        m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                        tr("Entered credentials are incorrect."),
                                        tr("Entered credentials are incorrect."));
      }
      else {
        m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                        tr("Other error occurred, contact developers."),
                                        tr("Other error occurred, contact developers."));
      }
    }
    else if (result.apiLevel() < XMPP_MINIMAL_API_LEVEL) {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                      tr("Installed version: %1, required at least: %2.")
                                        .arg(QString::number(result.apiLevel()),
                                             QString::number(XMPP_MINIMAL_API_LEVEL)),
                                      tr("Selected Tiny Tiny RSS server is running unsupported version of API."));
    }
    else {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                      tr("Installed version: %1, required at least: %2.")
                                        .arg(QString::number(result.apiLevel()),
                                             QString::number(XMPP_MINIMAL_API_LEVEL)),
                                      tr("Tiny Tiny RSS server is okay."));
    }
  }
  else if (factory.lastError() != QNetworkReply::NoError) {
    m_ui.m_lblTestResult
      ->setStatus(WidgetWithStatus::StatusType::Error,
                  tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(factory.lastError())),
                  tr("Network error, have you entered correct Tiny Tiny RSS API endpoint and password?"));
  }
  else {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                    tr("Unspecified error, did you enter correct URL?"),
                                    tr("Unspecified error, did you enter correct URL?"));
  }
*/
}

void XmppAccountDetails::onUsernameEdited() {
  QString domain = QXmppUtils::jidToDomain(m_ui.m_txtUsername->lineEdit()->text());

  if (!domain.isEmpty()) {
    m_ui.m_txtHost->lineEdit()->setText(domain);
  }
}

void XmppAccountDetails::onUsernameChanged() {
  const QString username = m_ui.m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void XmppAccountDetails::onPasswordChanged() {
  const QString password = m_ui.m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("Password is okay."));
  }
}

void XmppAccountDetails::onUrlChanged() {
  const QString url = m_ui.m_txtHost->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui.m_txtHost->setStatus(WidgetWithStatus::StatusType::Error, tr("URL cannot be empty."));
  }
  else {
    m_ui.m_txtHost->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is okay."));
  }
}
