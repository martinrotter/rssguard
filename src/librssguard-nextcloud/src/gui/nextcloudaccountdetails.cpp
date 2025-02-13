// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/nextcloudaccountdetails.h"

#include "src/definitions.h"
#include "src/nextcloudnetworkfactory.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/systemfactory.h>

NextcloudAccountDetails::NextcloudAccountDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_lblServerSideUpdateInformation
    ->setHelpText(tr("Leaving this option on causes that updates "
                     "of feeds will be probably much slower and may time-out often."),
                  true);
  m_ui.m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your Nextcloud account"));
  m_ui.m_txtPassword->lineEdit()->setPasswordMode(true);
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your Nextcloud account"));
  m_ui.m_txtUrl->lineEdit()->setPlaceholderText(tr("URL of your Nextcloud server, without any API path"));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test done yet."),
                                  tr("Here, results of connection test are shown."));

  connect(m_ui.m_spinLimitMessages,
          QOverload<int>::of(&QSpinBox::valueChanged),
          this,
          [=](int value) {
            if (value <= 0) {
              m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("= unlimited"));
            }
            else {
              m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("articles"));
            }
          });

  connect(m_ui.m_txtPassword->lineEdit(),
          &BaseLineEdit::textChanged,
          this,
          &NextcloudAccountDetails::onPasswordChanged);
  connect(m_ui.m_txtUsername->lineEdit(),
          &BaseLineEdit::textChanged,
          this,
          &NextcloudAccountDetails::onUsernameChanged);
  connect(m_ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &NextcloudAccountDetails::onUrlChanged);

  setTabOrder(m_ui.m_txtUrl->lineEdit(), m_ui.m_checkDownloadOnlyUnreadMessages);
  setTabOrder(m_ui.m_checkDownloadOnlyUnreadMessages, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_checkServerSideUpdate);
  setTabOrder(m_ui.m_checkServerSideUpdate, m_ui.m_txtUsername->lineEdit());
  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtPassword->lineEdit());
  setTabOrder(m_ui.m_txtPassword->lineEdit(), m_ui.m_btnTestSetup);

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
}

void NextcloudAccountDetails::performTest(const QNetworkProxy& custom_proxy) {
  NextcloudNetworkFactory factory;

  factory.setAuthUsername(m_ui.m_txtUsername->lineEdit()->text());
  factory.setAuthPassword(m_ui.m_txtPassword->lineEdit()->text());
  factory.setUrl(m_ui.m_txtUrl->lineEdit()->text());
  factory.setForceServerSideUpdate(m_ui.m_checkServerSideUpdate->isChecked());

  NextcloudStatusResponse result = factory.status(custom_proxy);

  if (result.networkError() != QNetworkReply::NetworkError::NoError) {
    m_ui.m_lblTestResult
      ->setStatus(WidgetWithStatus::StatusType::Error,
                  tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(result.networkError())),
                  tr("Network error, have you entered correct Nextcloud endpoint and password?"));
  }
  else if (result.isLoaded()) {
    if (!SystemFactory::isVersionEqualOrNewer(result.version(), QSL(NEXTCLOUD_MIN_VERSION))) {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                      tr("Installed version: %1, required at least: %2.")
                                        .arg(result.version(), QSL(NEXTCLOUD_MIN_VERSION)),
                                      tr("Selected Nextcloud News server is running unsupported version."));
    }
    else {
      m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                      tr("Installed version: %1, required at least: %2.")
                                        .arg(result.version(), QSL(NEXTCLOUD_MIN_VERSION)),
                                      tr("Nextcloud News server is okay."));
    }
  }
  else {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                    tr("Unspecified error, did you enter correct URL?"),
                                    tr("Unspecified error, did you enter correct URL?"));
  }
}

void NextcloudAccountDetails::onUsernameChanged() {
  const QString username = m_ui.m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Username is okay."));
  }
}

void NextcloudAccountDetails::onPasswordChanged() {
  const QString password = m_ui.m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("Password is okay."));
  }
}

void NextcloudAccountDetails::onUrlChanged() {
  const QString url = m_ui.m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("URL cannot be empty."));
  }
  else {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is okay."));
  }
}
