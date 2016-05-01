// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/toolbareditor.h"

#include "gui/basetoolbar.h"
#include "gui/dialogs/formmain.h"

#include <QKeyEvent>


ToolBarEditor::ToolBarEditor(QWidget *parent)
  : QWidget(parent), m_ui(new Ui::ToolBarEditor) {
  m_ui->setupUi(this);

  // Create connections.
  connect(m_ui->m_btnInsertSeparator, SIGNAL(clicked()), this, SLOT(insertSeparator()));
  connect(m_ui->m_btnInsertSpacer, SIGNAL(clicked()), this, SLOT(insertSpacer()));

  connect(m_ui->m_btnAddSelectedAction, SIGNAL(clicked()), this, SLOT(addSelectedAction()));
  connect(m_ui->m_btnDeleteAllActions, SIGNAL(clicked()), this, SLOT(deleteAllActions()));
  connect(m_ui->m_btnDeleteSelectedAction, SIGNAL(clicked()), this, SLOT(deleteSelectedAction()));
  connect(m_ui->m_btnMoveActionUp, SIGNAL(clicked()), this, SLOT(moveActionUp()));
  connect(m_ui->m_btnMoveActionDown, SIGNAL(clicked()), this, SLOT(moveActionDown()));

  connect(m_ui->m_listAvailableActions, SIGNAL(itemSelectionChanged()), this, SLOT(updateActionsAvailability()));
  connect(m_ui->m_listActivatedActions, SIGNAL(itemSelectionChanged()), this, SLOT(updateActionsAvailability()));
  connect(m_ui->m_listActivatedActions, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(deleteSelectedAction()));
  connect(m_ui->m_listAvailableActions, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(addSelectedAction()));

  m_ui->m_listActivatedActions->installEventFilter(this);
}

ToolBarEditor::~ToolBarEditor() {
  qDebug("Destroying ToolBarEditor instance.");
}

void ToolBarEditor::loadFromToolBar(BaseBar *tool_bar) {
  m_toolBar = tool_bar;

  QList<QAction*> activated_actions = m_toolBar->changeableActions();
  QList<QAction*> available_actions = m_toolBar->availableActions();

  foreach (const QAction *action, activated_actions) {
    QListWidgetItem *action_item = new QListWidgetItem(action->icon(), action->text().replace('&', ""), m_ui->m_listActivatedActions);

    if (action->isSeparator()) {
      action_item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
      action_item->setIcon(qApp->icons()->fromTheme(QSL("insert-object")));
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
      QListWidgetItem *action_item = new QListWidgetItem(action->icon(), action->text().replace('&', ""), m_ui->m_listAvailableActions);

      if (action->isSeparator()) {
        action_item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
        action_item->setText(tr("Separator"));
        action_item->setToolTip(tr("Separator"));
        action_item->setIcon(qApp->icons()->fromTheme(QSL("insert-object")));
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

  m_ui->m_listAvailableActions->sortItems(Qt::AscendingOrder);
  m_ui->m_listAvailableActions->setCurrentRow(0);
  m_ui->m_listActivatedActions->setCurrentRow(m_ui->m_listActivatedActions->count() >= 0 ? 0 : -1);
}

void ToolBarEditor::saveToolBar() {
  QStringList action_names;

  for (int i = 0; i < m_ui->m_listActivatedActions->count(); i++) {
    action_names.append(m_ui->m_listActivatedActions->item(i)->data(Qt::UserRole).toString());
  }

  m_toolBar->saveChangeableActions(action_names);
}

bool ToolBarEditor::eventFilter(QObject *object, QEvent *event) {
  if (object == m_ui->m_listActivatedActions) {
    if (event->type() == QEvent::KeyPress) {
      const QKeyEvent *key_event = static_cast<QKeyEvent*>(event);

      if (key_event->key() == Qt::Key_Delete) {
        deleteSelectedAction();
        return true;
      }
      else if (key_event->key() == Qt::Key_Down && key_event->modifiers() & Qt::ControlModifier) {
        moveActionDown();
        return true;
      }
      else if (key_event->key() == Qt::Key_Up && key_event->modifiers() & Qt::ControlModifier) {
        moveActionUp();
        return true;
      }
    }
  }

  return false;
}

void ToolBarEditor::updateActionsAvailability() {
  m_ui->m_btnDeleteAllActions->setEnabled(m_ui->m_listActivatedActions->count() > 0);
  m_ui->m_btnDeleteSelectedAction->setEnabled(m_ui->m_listActivatedActions->selectedItems().size() == 1);
  m_ui->m_btnMoveActionUp->setEnabled(m_ui->m_listActivatedActions->selectedItems().size() == 1 &&
                                      m_ui->m_listActivatedActions->currentRow() > 0);
  m_ui->m_btnMoveActionDown->setEnabled(m_ui->m_listActivatedActions->selectedItems().size() == 1 &&
                                        m_ui->m_listActivatedActions->currentRow() < m_ui->m_listActivatedActions->count() - 1);
  m_ui->m_btnAddSelectedAction->setEnabled(m_ui->m_listAvailableActions->selectedItems().size() > 0);
}

void ToolBarEditor::insertSpacer() {
  const int current_row = m_ui->m_listActivatedActions->currentRow();
  QListWidgetItem *item = new QListWidgetItem(tr("Toolbar spacer"));

  item->setIcon(qApp->icons()->fromTheme(QSL("go-jump")));
  item->setData(Qt::UserRole, SPACER_ACTION_NAME);

  m_ui->m_listActivatedActions->insertItem(current_row + 1, item);
  m_ui->m_listActivatedActions->setCurrentRow(current_row + 1);
}

void ToolBarEditor::insertSeparator() {
  const int current_row = m_ui->m_listActivatedActions->currentRow();
  QListWidgetItem *item = new QListWidgetItem(tr("Separator"));

  item->setData(Qt::UserRole, SEPARATOR_ACTION_NAME);
  item->setToolTip(tr("Separator"));
  item->setIcon(qApp->icons()->fromTheme(QSL("insert-object")));

  m_ui->m_listActivatedActions->insertItem(current_row + 1, item);
  m_ui->m_listActivatedActions->setCurrentRow(current_row + 1);
}

void ToolBarEditor::moveActionDown() {
  QList<QListWidgetItem*> items = m_ui->m_listActivatedActions->selectedItems();

  if (items.size() == 1 && m_ui->m_listActivatedActions->currentRow() < m_ui->m_listActivatedActions->count() - 1) {
    QListWidgetItem *selected_item = items.at(0);
    int row = m_ui->m_listActivatedActions->row(selected_item);

    m_ui->m_listActivatedActions->takeItem(row++);
    m_ui->m_listActivatedActions->insertItem(row, selected_item);
    m_ui->m_listActivatedActions->setCurrentRow(row);
  }
}

void ToolBarEditor::moveActionUp() {
  QList<QListWidgetItem*> items = m_ui->m_listActivatedActions->selectedItems();

  if (items.size() == 1 && m_ui->m_listActivatedActions->currentRow() > 0) {
    QListWidgetItem *selected_item = items.at(0);
    int row = m_ui->m_listActivatedActions->row(selected_item);

    m_ui->m_listActivatedActions->takeItem(row--);
    m_ui->m_listActivatedActions->insertItem(row, selected_item);
    m_ui->m_listActivatedActions->setCurrentRow(row);
  }
}

void ToolBarEditor::addSelectedAction() {
  QList<QListWidgetItem*> items = m_ui->m_listAvailableActions->selectedItems();

  if (items.size() == 1) {
    QListWidgetItem *selected_item = items.at(0);

    m_ui->m_listActivatedActions->insertItem(
          m_ui->m_listActivatedActions->currentRow() + 1,
          m_ui->m_listAvailableActions->takeItem(m_ui->m_listAvailableActions->row(selected_item)));
    m_ui->m_listActivatedActions->setCurrentRow(m_ui->m_listActivatedActions->currentRow() + 1);
  }
}

void ToolBarEditor::deleteSelectedAction() {
  QList<QListWidgetItem*> items = m_ui->m_listActivatedActions->selectedItems();

  if (items.size() == 1) {
    QListWidgetItem *selected_item = items.at(0);
    const QString data_item = selected_item->data(Qt::UserRole).toString();

    if (data_item == SEPARATOR_ACTION_NAME || data_item == SPACER_ACTION_NAME) {
      m_ui->m_listActivatedActions->takeItem(m_ui->m_listActivatedActions->row(selected_item));

      updateActionsAvailability();
    }
    else {
      m_ui->m_listAvailableActions->insertItem(
            m_ui->m_listAvailableActions->currentRow() + 1,
            m_ui->m_listActivatedActions->takeItem(m_ui->m_listActivatedActions->row(selected_item)));
      m_ui->m_listAvailableActions->sortItems(Qt::AscendingOrder);
      m_ui->m_listAvailableActions->setCurrentRow(m_ui->m_listAvailableActions->currentRow() + 1);
    }
  }
}

void ToolBarEditor::deleteAllActions() {
  QListWidgetItem *taken_item;
  QString data_item;

  while ((taken_item = m_ui->m_listActivatedActions->takeItem(0)) != 0) {
    data_item = taken_item->data(Qt::UserRole).toString();

    if (data_item != SEPARATOR_ACTION_NAME && data_item != SPACER_ACTION_NAME) {
      m_ui->m_listAvailableActions->insertItem(m_ui->m_listAvailableActions->currentRow() + 1, taken_item);
    }
  }

  m_ui->m_listAvailableActions->sortItems(Qt::AscendingOrder);
  updateActionsAvailability();
}
