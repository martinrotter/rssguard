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
  QList<QAction*> available_actions = m_toolBar->availableActions();

  foreach (QAction *action, activated_actions) {
    QListWidgetItem *action_item = new QListWidgetItem(action->icon(),
                                                       action->text().replace('&', ""),
                                                       m_ui->m_listActivatedActions);

    if (action->isSeparator()) {
      action_item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
      action_item->setText(tr("separator"));
    }
    else if (action->property("type").isValid()) {
      action_item->setData(Qt::UserRole, action->property("type").toString());
      action_item->setText(action->property("name").toString());
    }
    else {
      action_item->setData(Qt::UserRole, action->objectName());
    }
  }

  foreach (QAction *action, available_actions) {
    if (!activated_actions.contains(action)) {
      QListWidgetItem *action_item = new QListWidgetItem(action->icon(),
                                                         action->text().replace('&', ""),
                                                         m_ui->m_listAvailableActions);

      if (action->isSeparator()) {
        action_item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
        action_item->setText(tr("separator"));
      }
      else if (action->property("type").isValid()) {
        action_item->setData(Qt::UserRole, action->property("type").toString());
        action_item->setText(action->property("name").toString());
      }
      else {
        action_item->setData(Qt::UserRole, action->objectName());
      }
    }
  }
}

void ToolBarEditor::saveToolBar() {
  // TODO: ulozit actiony nastaveny v tomdl
  // e nastavovacim dialogu do prirazenyho toolbaru
  QStringList action_names;

  for (int i = 0; i < m_ui->m_listActivatedActions->count(); i++) {
    action_names.append(m_ui->m_listActivatedActions->item(i)->data(Qt::UserRole).toString());
  }

  m_toolBar->saveChangeableActions(action_names);
}
