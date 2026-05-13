// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formaskauth.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QPushButton>
#include <QScrollBar>

FormAskauth::FormAskauth(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("dialog-password")), tr("Application log"));

  m_ui.m_txtPassword->lineEdit()->setPasswordMode(true);

  setTabOrder(m_ui.m_txtUsername, m_ui.m_txtPassword);
  setTabOrder(m_ui.m_txtPassword, m_ui.m_btnBox);

  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textEdited, this, &FormAskauth::onUsernameChanged);
  connect(m_ui.m_txtPassword->lineEdit(), &BaseLineEdit::textEdited, this, &FormAskauth::onPasswordChanged);

  onUsernameChanged({});
  onPasswordChanged({});
}

FormAskauth::~FormAskauth() {}

QPair<QString, QString> FormAskauth::getUsernamePassword(const QString& title) {
  FormAskauth form(qApp->mainFormWidget());

  form.setWindowTitle(title);

  if (form.exec() == QDialog::DialogCode::Accepted) {
    return {form.m_ui.m_txtUsername->lineEdit()->text(), form.m_ui.m_txtPassword->lineEdit()->text()};
  }
  else {
    throw ApplicationException(tr("operation was cancelled"));
  }
}

void FormAskauth::onUsernameChanged(const QString& username) {
  if (username.trimmed().isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Warning, tr("Username should not be empty."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("OK."));
  }
}

void FormAskauth::onPasswordChanged(const QString& password) {
  if (password.trimmed().isEmpty()) {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Warning, tr("Password should not be empty."));
  }
  else {
    m_ui.m_txtPassword->setStatus(WidgetWithStatus::StatusType::Ok, tr("OK."));
  }
}

void FormAskauth::closeEvent(QCloseEvent* event) {
  reject();
  QDialog::closeEvent(event);
}

void FormAskauth::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::StandardKey::Cancel)) {
    m_ui.m_btnBox->button(QDialogButtonBox::StandardButton::Cancel)->click();
  }
  else {
    QDialog::keyPressEvent(event);
  }
}
