#include "gui/toolbareditor.h"

#include "gui/basetoolbar.h"


ToolBarEditor::ToolBarEditor(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::ToolBarEditor) {
  m_ui->setupUi(this);
}

ToolBarEditor::~ToolBarEditor() {
  delete m_ui;
}

void ToolBarEditor::loadFromToolBar(BaseToolBar* tool_bar) {
  m_toolBar = tool_bar;

  // TODO: nastavit dialog podle toolbaru
}

void ToolBarEditor::saveToolBar() {
  // TODO: ulozit actiony nastaveny v tomdl
  // e nastavovacim dialogu do prirazenyho toolbaru
}
