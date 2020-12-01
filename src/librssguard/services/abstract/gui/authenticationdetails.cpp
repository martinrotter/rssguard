// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/authenticationdetails.h"

AuthenticationDetails::AuthenticationDetails(QWidget* parent) : QWidget(parent) {
  setupUi(this);

  // Set text boxes.
  m_txtUsername->lineEdit()->setPlaceholderText(tr("Username"));
  m_txtUsername->lineEdit()->setToolTip(tr("Set username to access the feed."));
  m_txtPassword->lineEdit()->setPlaceholderText(tr("Password"));
  m_txtPassword->lineEdit()->setToolTip(tr("Set password to access the feed."));

  connect(m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &AuthenticationDetails::onUsernameChanged);
  connect(m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &AuthenticationDetails::onPasswordChanged);
  connect(m_gbAuthentication, &QGroupBox::toggled, this, &AuthenticationDetails::onAuthenticationSwitched);

  onUsernameChanged(QString());
  onPasswordChanged(QString());
}

void AuthenticationDetails::onUsernameChanged(const QString& new_username) {
  bool is_username_ok = !m_gbAuthentication->isChecked() || !new_username.simplified().isEmpty();

  m_txtUsername->setStatus(is_username_ok ?
                           LineEditWithStatus::StatusType::Ok :
                           LineEditWithStatus::StatusType::Warning,
                           is_username_ok ?
                           tr("Username is ok or it is not needed.") :
                           tr("Username is empty."));
}

void AuthenticationDetails::onPasswordChanged(const QString& new_password) {
  bool is_password_ok = !m_gbAuthentication->isChecked() || !new_password.simplified().isEmpty();

  m_txtPassword->setStatus(is_password_ok ?
                           LineEditWithStatus::StatusType::Ok :
                           LineEditWithStatus::StatusType::Warning,
                           is_password_ok ?
                           tr("Password is ok or it is not needed.") :
                           tr("Password is empty."));
}

void AuthenticationDetails::onAuthenticationSwitched() {
  onUsernameChanged(m_txtUsername->lineEdit()->text());
  onPasswordChanged(m_txtPassword->lineEdit()->text());
}
