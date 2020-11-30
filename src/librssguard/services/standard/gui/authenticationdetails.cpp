// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/authenticationdetails.h"

AuthenticationDetails::AuthenticationDetails(QWidget* parent) : QWidget(parent) {
  ui.setupUi(this);

  // Set text boxes.
  ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("Username"));
  ui.m_txtUsername->lineEdit()->setToolTip(tr("Set username to access the feed."));
  ui.m_txtPassword->lineEdit()->setPlaceholderText(tr("Password"));
  ui.m_txtPassword->lineEdit()->setToolTip(tr("Set password to access the feed."));

  connect(ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &AuthenticationDetails::onUsernameChanged);
  connect(ui.m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &AuthenticationDetails::onPasswordChanged);
  connect(ui.m_gbAuthentication, &QGroupBox::toggled, this, &AuthenticationDetails::onAuthenticationSwitched);

  onUsernameChanged(QString());
  onPasswordChanged(QString());
}

void AuthenticationDetails::onUsernameChanged(const QString& new_username) {
  bool is_username_ok = !ui.m_gbAuthentication->isChecked() || !new_username.simplified().isEmpty();

  ui.m_txtUsername->setStatus(is_username_ok ?
                              LineEditWithStatus::StatusType::Ok :
                              LineEditWithStatus::StatusType::Warning,
                              is_username_ok ?
                              tr("Username is ok or it is not needed.") :
                              tr("Username is empty."));
}

void AuthenticationDetails::onPasswordChanged(const QString& new_password) {
  bool is_password_ok = !ui.m_gbAuthentication->isChecked() || !new_password.simplified().isEmpty();

  ui.m_txtPassword->setStatus(is_password_ok ?
                              LineEditWithStatus::StatusType::Ok :
                              LineEditWithStatus::StatusType::Warning,
                              is_password_ok ?
                              tr("Password is ok or it is not needed.") :
                              tr("Password is empty."));
}

void AuthenticationDetails::onAuthenticationSwitched() {
  onUsernameChanged(ui.m_txtUsername->lineEdit()->text());
  onPasswordChanged(ui.m_txtPassword->lineEdit()->text());
}
