// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formaddeditlabel.h"

#include "services/abstract/label.h"

FormAddEditLabel::FormAddEditLabel(QWidget* parent) : QDialog(parent), ui(new Ui::FormAddEditLabel) {
  ui->setupUi(this);
}

FormAddEditLabel::~FormAddEditLabel() {
  delete ui;
}

Label* FormAddEditLabel::execForAdd() {
  auto exit_code = exec();

  return nullptr;
}
