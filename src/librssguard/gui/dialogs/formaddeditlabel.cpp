// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formaddeditlabel.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/label.h"

FormAddEditLabel::FormAddEditLabel(QWidget* parent) : QDialog(parent), m_editableLabel(nullptr) {
  m_ui.setupUi(this);
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

  m_ui.m_txtName->lineEdit()->setText(tr("Hot stuff"));
}

Label* FormAddEditLabel::execForAdd() {
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("tag-new")), tr("Create new label"));
  m_ui.m_btnColor->setRandomColor();

  auto exit_code = exec();

  if (exit_code == QDialog::DialogCode::Accepted) {
    return new Label(m_ui.m_txtName->lineEdit()->text(), m_ui.m_btnColor->color());
  }
  else {
    return nullptr;
  }
}

bool FormAddEditLabel::execForEdit(Label* lbl) {
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("tag-properties")), tr("Edit label '%1'").arg(lbl->title()));

  m_editableLabel = lbl;
  m_ui.m_btnColor->setColor(lbl->color());
  m_ui.m_txtName->lineEdit()->setText(lbl->title());

  auto exit_code = exec();

  if (exit_code == QDialog::DialogCode::Accepted) {
    // TODO: Place server-side changes perhaps to here?
    m_editableLabel->setColor(m_ui.m_btnColor->color());
    m_editableLabel->setTitle(m_ui.m_txtName->lineEdit()->text());
    return true;
  }
  else {
    return false;
  }
}
