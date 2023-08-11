// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formaddeditprobe.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/search.h"

FormAddEditProbe::FormAddEditProbe(QWidget* parent) : QDialog(parent), m_editableProbe(nullptr) {
  m_ui.setupUi(this);
  m_ui.m_txtName->lineEdit()->setPlaceholderText(tr("Name for your query"));
  m_ui.m_txtFilter->lineEdit()->setPlaceholderText(tr("Regular expression"));

  connect(m_ui.m_txtName->lineEdit(), &QLineEdit::textChanged, this, [this](const QString& text) {
    if (text.isEmpty()) {
      m_ui.m_txtName->setStatus(LineEditWithStatus::StatusType::Error, tr("Regex query name cannot be empty."));
    }
    else {
      m_ui.m_txtName->setStatus(LineEditWithStatus::StatusType::Ok, tr("Perfect!"));
    }
  });

  connect(m_ui.m_txtFilter->lineEdit(), &QLineEdit::textChanged, this, [this](const QString& text) {
    if (text.isEmpty()) {
      m_ui.m_txtFilter->setStatus(LineEditWithStatus::StatusType::Error, tr("Regular expression cannot be empty."));
    }
    else if (!QRegularExpression(text).isValid()) {
      m_ui.m_txtFilter->setStatus(LineEditWithStatus::StatusType::Error, tr("Regular expression is not well-formed."));
    }
    else {
      m_ui.m_txtFilter->setStatus(LineEditWithStatus::StatusType::Ok, tr("Perfect!"));
    }
  });

  emit m_ui.m_txtName->lineEdit()->textChanged({});
  emit m_ui.m_txtFilter->lineEdit()->textChanged({});
}

Search* FormAddEditProbe::execForAdd() {
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("tag-new")), tr("Create new regex query"));

  m_ui.m_btnColor->setRandomColor();
  m_ui.m_txtName->lineEdit()->setText(tr("Hot stuff"));
  m_ui.m_txtFilter->setFocus();

  auto exit_code = exec();

  if (exit_code == QDialog::DialogCode::Accepted) {
    return new Search(m_ui.m_txtName->lineEdit()->text(),
                      m_ui.m_txtFilter->lineEdit()->text(),
                      m_ui.m_btnColor->color());
  }
  else {
    return nullptr;
  }
}

bool FormAddEditProbe::execForEdit(Search* prb) {
  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->fromTheme(QSL("tag-properties")),
                                      tr("Edit regex query '%1'").arg(prb->title()));

  m_editableProbe = prb;

  m_ui.m_btnColor->setColor(prb->color());
  m_ui.m_txtName->lineEdit()->setText(prb->title());
  m_ui.m_txtFilter->lineEdit()->setText(prb->filter());
  m_ui.m_txtFilter->setFocus();

  auto exit_code = exec();

  if (exit_code == QDialog::DialogCode::Accepted) {
    m_editableProbe->setColor(m_ui.m_btnColor->color());
    m_editableProbe->setFilter(m_ui.m_txtFilter->lineEdit()->text());
    m_editableProbe->setTitle(m_ui.m_txtName->lineEdit()->text());
    return true;
  }
  else {
    return false;
  }
}
