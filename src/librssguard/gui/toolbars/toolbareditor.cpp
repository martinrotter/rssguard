// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/toolbars/toolbareditor.h"

#include "definitions/globals.h"
#include "gui/toolbars/basetoolbar.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QKeyEvent>
#include <QWidgetAction>

ToolBarEditor::ToolBarEditor(QWidget* parent) : QWidget(parent), m_ui(new Ui::ToolBarEditor), m_toolBar(nullptr) {
  m_ui->setupUi(this);

  // Create connections.
  connect(m_ui->m_btnInsertSeparator, &QToolButton::clicked, this, &ToolBarEditor::insertSeparator);
  connect(m_ui->m_btnInsertSpacer, &QToolButton::clicked, this, &ToolBarEditor::insertSpacer);
  connect(m_ui->m_btnAddSelectedAction, &QToolButton::clicked, this, &ToolBarEditor::addSelectedAction);
  connect(m_ui->m_btnDeleteAllActions, &QToolButton::clicked, this, &ToolBarEditor::deleteAllActions);
  connect(m_ui->m_btnDeleteSelectedAction, &QToolButton::clicked, this, &ToolBarEditor::deleteSelectedAction);
  connect(m_ui->m_btnMoveActionUp, &QToolButton::clicked, this, &ToolBarEditor::moveActionUp);
  connect(m_ui->m_btnMoveActionDown, &QToolButton::clicked, this, &ToolBarEditor::moveActionDown);
  connect(m_ui->m_btnReset, &QToolButton::clicked, this, &ToolBarEditor::resetToolBar);
  connect(m_ui->m_listAvailableActions,
          &QListWidget::itemSelectionChanged,
          this,
          &ToolBarEditor::updateActionsAvailability);
  connect(m_ui->m_listActivatedActions,
          &QListWidget::itemSelectionChanged,
          this,
          &ToolBarEditor::updateActionsAvailability);
  connect(m_ui->m_listActivatedActions, &QListWidget::itemDoubleClicked, this, &ToolBarEditor::deleteSelectedAction);
  connect(m_ui->m_listAvailableActions, &QListWidget::itemDoubleClicked, this, &ToolBarEditor::addSelectedAction);

  m_ui->m_listActivatedActions->installEventFilter(this);
  m_ui->m_btnInsertSeparator->setIcon(qApp->icons()->fromTheme(QSL("insert-object"), QSL("insert-page-break")));
  m_ui->m_btnInsertSpacer->setIcon(qApp->icons()->fromTheme(QSL("go-jump")));
  m_ui->m_btnAddSelectedAction->setIcon(qApp->icons()->fromTheme(QSL("go-previous")));
  m_ui->m_btnDeleteAllActions->setIcon(qApp->icons()->fromTheme(QSL("application-exit")));
  m_ui->m_btnDeleteSelectedAction->setIcon(qApp->icons()->fromTheme(QSL("go-next")));
  m_ui->m_btnMoveActionDown->setIcon(qApp->icons()->fromTheme(QSL("down"), QSL("arrow-down")));
  m_ui->m_btnMoveActionUp->setIcon(qApp->icons()->fromTheme(QSL("up"), QSL("arrow-up")));
  m_ui->m_btnReset->setIcon(qApp->icons()->fromTheme(QSL("reload"), QSL("edit-reset")));
}

void ToolBarEditor::loadFromToolBar(BaseBar* tool_bar) {
  m_toolBar = tool_bar;

  QList<QAction*> activated_actions = m_toolBar->activatedActions();
  QList<QAction*> available_actions = m_toolBar->availableActions();

  loadEditor(activated_actions, available_actions);
}

void ToolBarEditor::saveToolBar() {
  QStringList action_names;

  for (int i = 0; i < m_ui->m_listActivatedActions->count(); i++) {
    action_names.append(m_ui->m_listActivatedActions->item(i)->data(Qt::UserRole).toString());
  }

  m_toolBar->saveAndSetActions(action_names);
}

void ToolBarEditor::resetToolBar() {
  if (m_toolBar != nullptr) {
    loadEditor(m_toolBar->convertActions(m_toolBar->defaultActions()), m_toolBar->availableActions());
    emit setupChanged();
  }
}

void ToolBarEditor::loadEditor(const QList<QAction*>& activated_actions, const QList<QAction*>& available_actions) {
  m_ui->m_listActivatedActions->clear();
  m_ui->m_listAvailableActions->clear();

  for (const QAction* action : activated_actions) {
    QListWidgetItem* action_item =
      new QListWidgetItem(action->icon(), action->text().replace('&', QL1S("")), m_ui->m_listActivatedActions);

    if (action->isSeparator()) {
      action_item->setData(Qt::ItemDataRole::UserRole, SEPARATOR_ACTION_NAME);
      action_item->setIcon(qApp->icons()->fromTheme(QSL("draw-line"), QSL("insert-object")));
      action_item->setText(tr("Separator"));
      action_item->setToolTip(tr("Separator"));
    }
    else if (action->property("type").isValid()) {
      action_item->setData(Qt::ItemDataRole::UserRole, action->property("type").toString());
      action_item->setText(action->property("name").toString());
      action_item->setToolTip(action_item->text());
    }
    else {
      action_item->setData(Qt::ItemDataRole::UserRole, action->objectName());
      action_item->setToolTip(action->toolTip());
    }

    if (auto widgetAction = qobject_cast<const QWidgetAction*>(action); widgetAction) {
      if (auto toolButton = qobject_cast<const QToolButton*>(widgetAction->defaultWidget()); toolButton) {
        if (const QAction* action = toolButton->defaultAction(); action) {
          const QString objectName = action->objectName();
          const QString name = action_item->data(Qt::ItemDataRole::UserRole).toString() +
                               (objectName.isEmpty() ? "" : "[" + objectName.toStdString() + "]").c_str();

          action_item->setData(Qt::ItemDataRole::UserRole, name);
        }
      }
    }
  }

  for (QAction* action : available_actions) {
    if (!activated_actions.contains(action)) {
      QListWidgetItem* action_item =
        new QListWidgetItem(action->icon(), action->text().replace('&', QL1S("")), m_ui->m_listAvailableActions);

      if (action->isSeparator()) {
        action_item->setData(Qt::ItemDataRole::UserRole, QSL(SEPARATOR_ACTION_NAME));
        action_item->setText(tr("Separator"));
        action_item->setToolTip(tr("Separator"));
        action_item->setIcon(qApp->icons()->fromTheme(QSL("insert-object")));
      }
      else if (action->property("type").isValid()) {
        action_item->setData(Qt::ItemDataRole::UserRole, action->property("type").toString());
        action_item->setText(action->property("name").toString());
        action_item->setToolTip(action_item->text());
      }
      else {
        action_item->setData(Qt::ItemDataRole::UserRole, action->objectName());
        action_item->setToolTip(action->toolTip());
      }
    }
  }

  m_ui->m_listAvailableActions->sortItems(Qt::SortOrder::AscendingOrder);
  m_ui->m_listAvailableActions->setCurrentRow(m_ui->m_listAvailableActions->count() >= 0 ? 0 : -1);
  m_ui->m_listActivatedActions->setCurrentRow(m_ui->m_listActivatedActions->count() >= 0 ? 0 : -1);
}

bool ToolBarEditor::eventFilter(QObject* object, QEvent* event) {
  if (object == m_ui->m_listActivatedActions) {
    if (event->type() == QEvent::Type::KeyPress) {
      const auto* key_event = static_cast<QKeyEvent*>(event);

      if (key_event->key() == Qt::Key::Key_Delete) {
        deleteSelectedAction();
        return true;
      }
      else if (key_event->key() == Qt::Key::Key_Down &&
               Globals::hasFlag(key_event->modifiers(), Qt::KeyboardModifier::ControlModifier)) {
        moveActionDown();
        return true;
      }
      else if (key_event->key() == Qt::Key::Key_Up &&
               Globals::hasFlag(key_event->modifiers(), Qt::KeyboardModifier::ControlModifier)) {
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
                                        m_ui->m_listActivatedActions->currentRow() <
                                          m_ui->m_listActivatedActions->count() - 1);
  m_ui->m_btnAddSelectedAction->setEnabled(m_ui->m_listAvailableActions->selectedItems().size() > 0);
}

void ToolBarEditor::insertSpacer() {
  const int current_row = m_ui->m_listActivatedActions->currentRow();
  auto* item = new QListWidgetItem(tr("Toolbar spacer"));

  item->setIcon(qApp->icons()->fromTheme(QSL("go-jump")));
  item->setData(Qt::ItemDataRole::UserRole, SPACER_ACTION_NAME);
  m_ui->m_listActivatedActions->insertItem(current_row + 1, item);
  m_ui->m_listActivatedActions->setCurrentRow(current_row + 1);
  emit setupChanged();
}

void ToolBarEditor::insertSeparator() {
  const int current_row = m_ui->m_listActivatedActions->currentRow();
  QListWidgetItem* item = new QListWidgetItem(tr("Separator"));

  item->setData(Qt::ItemDataRole::UserRole, SEPARATOR_ACTION_NAME);
  item->setToolTip(tr("Separator"));
  item->setIcon(qApp->icons()->fromTheme(QSL("insert-object")));
  m_ui->m_listActivatedActions->insertItem(current_row + 1, item);
  m_ui->m_listActivatedActions->setCurrentRow(current_row + 1);
  emit setupChanged();
}

void ToolBarEditor::moveActionDown() {
  QList<QListWidgetItem*> items = m_ui->m_listActivatedActions->selectedItems();

  if (items.size() == 1 && m_ui->m_listActivatedActions->currentRow() < m_ui->m_listActivatedActions->count() - 1) {
    QListWidgetItem* selected_item = items.at(0);
    int row = m_ui->m_listActivatedActions->row(selected_item);

    m_ui->m_listActivatedActions->takeItem(row++);
    m_ui->m_listActivatedActions->insertItem(row, selected_item);
    m_ui->m_listActivatedActions->setCurrentRow(row);
    emit setupChanged();
  }
}

void ToolBarEditor::moveActionUp() {
  QList<QListWidgetItem*> items = m_ui->m_listActivatedActions->selectedItems();

  if (items.size() == 1 && m_ui->m_listActivatedActions->currentRow() > 0) {
    QListWidgetItem* selected_item = items.at(0);
    int row = m_ui->m_listActivatedActions->row(selected_item);

    m_ui->m_listActivatedActions->takeItem(row--);
    m_ui->m_listActivatedActions->insertItem(row, selected_item);
    m_ui->m_listActivatedActions->setCurrentRow(row);
    emit setupChanged();
  }
}

void ToolBarEditor::addSelectedAction() {
  QList<QListWidgetItem*> items = m_ui->m_listAvailableActions->selectedItems();

  if (items.size() == 1) {
    QListWidgetItem* selected_item = items.at(0);

    m_ui->m_listActivatedActions
      ->insertItem(m_ui->m_listActivatedActions->currentRow() + 1,
                   m_ui->m_listAvailableActions->takeItem(m_ui->m_listAvailableActions->row(selected_item)));
    m_ui->m_listActivatedActions->setCurrentRow(m_ui->m_listActivatedActions->currentRow() + 1);
    emit setupChanged();
  }
}

void ToolBarEditor::deleteSelectedAction() {
  QList<QListWidgetItem*> items = m_ui->m_listActivatedActions->selectedItems();

  if (items.size() == 1) {
    QListWidgetItem* selected_item = items.at(0);
    const QString data_item = selected_item->data(Qt::ItemDataRole::UserRole).toString();

    if (data_item == QSL(SEPARATOR_ACTION_NAME) || data_item == QSL(SPACER_ACTION_NAME)) {
      m_ui->m_listActivatedActions->takeItem(m_ui->m_listActivatedActions->row(selected_item));
      updateActionsAvailability();
    }
    else {
      m_ui->m_listAvailableActions
        ->insertItem(m_ui->m_listAvailableActions->currentRow() + 1,
                     m_ui->m_listActivatedActions->takeItem(m_ui->m_listActivatedActions->row(selected_item)));
      m_ui->m_listAvailableActions->sortItems(Qt::SortOrder::AscendingOrder);
      m_ui->m_listAvailableActions->setCurrentRow(m_ui->m_listAvailableActions->currentRow() + 1);
    }

    emit setupChanged();
  }
}

void ToolBarEditor::deleteAllActions() {
  QListWidgetItem* taken_item;
  QString data_item;

  while ((taken_item = m_ui->m_listActivatedActions->takeItem(0)) != nullptr) {
    data_item = taken_item->data(Qt::ItemDataRole::UserRole).toString();

    if (data_item != QSL(SEPARATOR_ACTION_NAME) && data_item != QSL(SPACER_ACTION_NAME)) {
      m_ui->m_listAvailableActions->insertItem(m_ui->m_listAvailableActions->currentRow() + 1, taken_item);
    }
  }

  m_ui->m_listAvailableActions->sortItems(Qt::SortOrder::AscendingOrder);
  updateActionsAvailability();
  emit setupChanged();
}
