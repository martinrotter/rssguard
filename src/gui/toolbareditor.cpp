#include "gui/toolbareditor.h"

#include "gui/basetoolbar.h"
#include "gui/formmain.h"

#include <cstdint>


ToolBarEditor::ToolBarEditor(QWidget *parent)
  : QWidget(parent), m_ui(new Ui::ToolBarEditor) {
  m_ui->setupUi(this);
}

ToolBarEditor::~ToolBarEditor() {
  delete m_ui;
}

void ToolBarEditor::loadFromToolBar(BaseToolBar* tool_bar) {
  m_toolBar = tool_bar;

  QList<QAction*> activated_actions = m_toolBar->changeableActions();
  QList<QAction*> available_actions = FormMain::instance()->allActions().values();

  foreach (QAction *action, activated_actions) {
    QListWidgetItem *action_item = new QListWidgetItem(action->icon(),
                                                       action->text().replace('&', ""),
                                                       m_ui->m_listActivatedActions);
    action_item->setData(Qt::UserRole, QVariant::fromValue((intptr_t) action));
  }

  foreach (QAction *action, available_actions) {
    if (!activated_actions.contains(action)) {
      QListWidgetItem *action_item = new QListWidgetItem(action->icon(),
                                                         action->text().replace('&', ""),
                                                         m_ui->m_listAvailableActions);
      action_item->setData(Qt::UserRole, QVariant::fromValue((intptr_t) action));
    }
  }
}

void ToolBarEditor::saveToolBar() {
  // TODO: ulozit actiony nastaveny v tomdl
  // e nastavovacim dialogu do prirazenyho toolbaru
}
