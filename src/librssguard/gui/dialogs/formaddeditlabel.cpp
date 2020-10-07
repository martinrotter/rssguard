// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formaddeditlabel.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/label.h"

FormAddEditLabel::FormAddEditLabel(QWidget* parent) : QDialog(parent) {
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
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("tag-properties")), tr("Create new label"));
  m_ui.m_btnColor->setRandomColor();

  auto exit_code = exec();
  auto xxx = m_ui.m_btnColor->color().name();

  if (exit_code == QDialog::DialogCode::Accepted) {
    return new Label(m_ui.m_txtName->lineEdit()->text(), m_ui.m_btnColor->color());
  }
  else {
    return nullptr;
  }
}
