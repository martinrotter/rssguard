#include "gui/toolbareditor.h"


ToolBarEditor::ToolBarEditor(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::ToolBarEditor) {
  m_ui->setupUi(this);
}

ToolBarEditor::~ToolBarEditor() {
  delete m_ui;
}
