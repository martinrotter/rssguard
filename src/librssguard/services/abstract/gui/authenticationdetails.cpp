// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/authenticationdetails.h"

#include "definitions/definitions.h"

AuthenticationDetails::AuthenticationDetails(bool only_basic, QWidget* parent) : QWidget(parent) {
  setupUi(this);

  // Set text boxes.
  m_txtPassword->lineEdit()->setPasswordMode(true);
  m_txtUsername->lineEdit()->setPlaceholderText(tr("Username"));
  m_txtUsername->lineEdit()->setToolTip(tr("Set username to access the feed."));
  m_txtPassword->lineEdit()->setPlaceholderText(tr("Password"));
  m_txtPassword->lineEdit()->setToolTip(tr("Set password to access the feed."));

  m_cbAuthType->addItem(tr("No authentication"),
                        QVariant::fromValue(NetworkFactory::NetworkAuthentication::NoAuthentication));
  m_cbAuthType->addItem(tr("HTTP Basic"), QVariant::fromValue(NetworkFactory::NetworkAuthentication::Basic));

  if (!only_basic) {
    m_cbAuthType->addItem(tr("Token"), QVariant::fromValue(NetworkFactory::NetworkAuthentication::Token));
  }

  connect(m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &AuthenticationDetails::onUsernameChanged);
  connect(m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &AuthenticationDetails::onPasswordChanged);
  connect(m_cbAuthType,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &AuthenticationDetails::onAuthenticationSwitched);

  onAuthenticationSwitched();
}

void AuthenticationDetails::setAuthenticationType(NetworkFactory::NetworkAuthentication protect) {
  auto fnd = m_cbAuthType->findData(QVariant::fromValue(protect));

  if (fnd >= 0) {
    m_cbAuthType->setCurrentIndex(fnd);
  }
}

NetworkFactory::NetworkFactory::NetworkAuthentication AuthenticationDetails::authenticationType() const {
  return m_cbAuthType->currentData().value<NetworkFactory::NetworkAuthentication>();
}

void AuthenticationDetails::onUsernameChanged(const QString& new_username) {
  bool is_username_ok = authenticationType() == NetworkFactory::NetworkAuthentication::NoAuthentication ||
                        !new_username.simplified().isEmpty();

  m_txtUsername->setStatus(is_username_ok ? LineEditWithStatus::StatusType::Ok
                                          : LineEditWithStatus::StatusType::Warning,
                           is_username_ok ? tr("Username/token is ok or it is not needed.")
                                          : tr("Username/token is empty."));
}

void AuthenticationDetails::onPasswordChanged(const QString& new_password) {
  bool is_password_ok = authenticationType() == NetworkFactory::NetworkAuthentication::NoAuthentication ||
                        !new_password.simplified().isEmpty();

  m_txtPassword->setStatus(is_password_ok ? LineEditWithStatus::StatusType::Ok
                                          : LineEditWithStatus::StatusType::Warning,
                           is_password_ok ? tr("Password is ok or it is not needed.") : tr("Password is empty."));
}

void AuthenticationDetails::onAuthenticationSwitched() {
  onUsernameChanged(m_txtUsername->lineEdit()->text());
  onPasswordChanged(m_txtPassword->lineEdit()->text());

  auto prot = authenticationType();

  m_lblPassword->setVisible(prot != NetworkFactory::NetworkAuthentication::Token);
  m_txtPassword->setVisible(prot != NetworkFactory::NetworkAuthentication::Token);

  if (prot == NetworkFactory::NetworkAuthentication::Token) {
    m_lblUsername->setText(tr("Access token"));
  }
  else {
    m_lblUsername->setText(tr("Username"));
  }

  m_gbAuthentication->setEnabled(prot != NetworkFactory::NetworkAuthentication::NoAuthentication);
}
