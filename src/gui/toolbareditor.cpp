#include "gui/toolbareditor.h"

#include "gui/basetoolbar.h"
#include "gui/formmain.h"


ToolBarEditor::ToolBarEditor(QWidget *parent)
  : QWidget(parent), m_ui(new Ui::ToolBarEditor) {
  m_ui->setupUi(this);

  // Create connections.
  connect(m_ui->m_btnInsertSeparator, SIGNAL(clicked()), this, SLOT(insertSeparator()));
  connect(m_ui->m_btnInsertSpacer, SIGNAL(clicked()), this, SLOT(insertSpacer()));
}

ToolBarEditor::~ToolBarEditor() {
  delete m_ui;
}

void ToolBarEditor::loadFromToolBar(BaseToolBar* tool_bar) {
  m_toolBar = tool_bar;

  QList<QAction*> activated_actions = m_toolBar->changeableActions();
  QList<QAction*> available_actions = m_toolBar->availableActions().values();

  foreach (QAction *action, activated_actions) {
    QListWidgetItem *action_item = new QListWidgetItem(action->icon(),
                                                       action->text().replace('&', ""),
                                                       m_ui->m_listActivatedActions);

    if (action->isSeparator()) {
      action_item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
      action_item->setIcon(IconFactory::instance()->fromTheme("view-separator"));
      action_item->setText(tr("Separator"));
      action_item->setToolTip(tr("Separator"));
    }
    else if (action->property("type").isValid()) {
      action_item->setData(Qt::UserRole, action->property("type").toString());
      action_item->setText(action->property("name").toString());
      action_item->setToolTip(action_item->text());
    }
    else {
      action_item->setData(Qt::UserRole, action->objectName());
      action_item->setToolTip(action->toolTip());
    }
  }

  foreach (QAction *action, available_actions) {
    if (!activated_actions.contains(action)) {
      QListWidgetItem *action_item = new QListWidgetItem(action->icon(),
                                                         action->text().replace('&', ""),
                                                         m_ui->m_listAvailableActions);

      if (action->isSeparator()) {
        action_item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
        action_item->setText(tr("Separator"));
        action_item->setToolTip(tr("Separator"));
        action_item->setIcon(IconFactory::instance()->fromTheme("view-separator"));
      }
      else if (action->property("type").isValid()) {
        action_item->setData(Qt::UserRole, action->property("type").toString());
        action_item->setText(action->property("name").toString());
        action_item->setToolTip(action_item->text());
      }
      else {
        action_item->setData(Qt::UserRole, action->objectName());
        action_item->setToolTip(action->toolTip());
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

void ToolBarEditor::insertSpacer() {
  int current_row = m_ui->m_listActivatedActions->currentRow();

  QListWidgetItem *item = new QListWidgetItem(tr("Toolbar spacer"));
  item->setIcon(IconFactory::instance()->fromTheme("application-search"));
  item->setData(Qt::UserRole, SPACER_ACTION_NAME);

  if (current_row >= 0) {
    m_ui->m_listActivatedActions->insertItem(current_row + 1, item);
  }
  else {
    m_ui->m_listActivatedActions->addItem(item);
  }
}

void ToolBarEditor::insertSeparator() {
  int current_row = m_ui->m_listActivatedActions->currentRow();

  QListWidgetItem *item = new QListWidgetItem(tr("Separator"));
  item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
  item->setToolTip(tr("Separator"));
  item->setIcon(IconFactory::instance()->fromTheme("view-separator"));

  if (current_row >= 0) {
    m_ui->m_listActivatedActions->insertItem(current_row + 1, item);
  }
  else {
    m_ui->m_listActivatedActions->addItem(item);
  }
}
