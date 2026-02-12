// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formaddeditlabel.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/label.h"
#include "services/abstract/serviceroot.h"

#include <QPushButton>

FormAddEditLabel::FormAddEditLabel(QWidget* parent) : QDialog(parent), m_editableLabel(nullptr) {
  m_ui.setupUi(this);

  m_ui.m_btnColor->setColorOnlyMode(false);
  m_ui.m_txtName->lineEdit()->setPlaceholderText(tr("Name for your label"));

  connect(m_ui.m_txtName->lineEdit(), &QLineEdit::textChanged, this, [this](const QString& text) {
    m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(!text.isEmpty());

    if (text.isEmpty()) {
      m_ui.m_txtName->setStatus(LineEditWithStatus::StatusType::Error, tr("Label's name cannot be empty."));
    }
    else {
      m_ui.m_txtName->setStatus(LineEditWithStatus::StatusType::Ok, tr("Perfect!"));
    }
  });
}

Label* FormAddEditLabel::execForAdd(ServiceRoot* account) {
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("tag-new")), tr("Create new label"));

  m_ui.m_btnColor->setAdditionalIcons(account->getSubTreeIcons());
  m_ui.m_btnColor->setRandomColor();
  m_ui.m_txtName->lineEdit()->setText(tr("Hot stuff"));
  m_ui.m_txtName->setFocus();

  auto exit_code = exec();

  if (exit_code == QDialog::DialogCode::Accepted) {
    return new Label(m_ui.m_txtName->lineEdit()->text(), m_ui.m_btnColor->icon());
  }
  else {
    return nullptr;
  }
}

bool FormAddEditLabel::execForEdit(Label* lbl) {
  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->fromTheme(QSL("tag-properties")),
                                      tr("Edit label '%1'").arg(lbl->title()));

  m_editableLabel = lbl;

  m_ui.m_btnColor->setAdditionalIcons(lbl->account()->getSubTreeIcons());
  m_ui.m_btnColor->setIcon(lbl->icon());
  m_ui.m_txtName->lineEdit()->setText(lbl->title());
  m_ui.m_txtName->setFocus();

  auto exit_code = exec();

  if (exit_code == QDialog::DialogCode::Accepted) {
    m_editableLabel->setIcon(m_ui.m_btnColor->icon());
    m_editableLabel->setTitle(m_ui.m_txtName->lineEdit()->text());
    return true;
  }
  else {
    return false;
  }
}
