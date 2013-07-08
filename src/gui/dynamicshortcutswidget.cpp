#include <QGridLayout>
#include <QAction>
#include <QLabel>
#include <QSpacerItem>

#include "gui/dynamicshortcutswidget.h"
#include "gui/shortcutcatcher.h"


DynamicShortcutsWidget::DynamicShortcutsWidget(QWidget *parent) : QWidget(parent) {
  // Create layout for this control and set is as active.
  m_layout = new QGridLayout(this);
  m_layout->setMargin(0);

  setLayout(m_layout);
}

DynamicShortcutsWidget::~DynamicShortcutsWidget() {
  delete m_layout;
}

void DynamicShortcutsWidget::updateShortcuts() {
  foreach (ActionBinding binding, m_actionBindings) {
    binding.first->setShortcut(binding.second->shortcut());
  }
}

void DynamicShortcutsWidget::populate(const QList<QAction *> actions) {
  m_actionBindings.clear();

  // TODO: Make labels smaller correctly and review this method.
  int row_id = 0;
  bool second_column = false;

  foreach (QAction *action, actions) {
    // Create shortcut catcher for this action and set default shortcut.
    ShortcutCatcher *catcher = new ShortcutCatcher(this);
    catcher->setShortcut(action->shortcut());

    // Store information for re-initialization of shortcuts
    // of actions when widget gets "confirmed".
    QPair<QAction*,ShortcutCatcher*> new_binding;
    new_binding.first = action;
    new_binding.second = catcher;

    m_actionBindings << new_binding;

    // Add new catcher to our control.
    QLabel *label = new QLabel(this);
    label->setText(action->text().remove('&'));
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    if (second_column) {
      m_layout->addWidget(label, row_id, 2);
      m_layout->addWidget(catcher, row_id, 3);
      second_column = false;

      // Continue to the next row.
      row_id++;
    }
    else {
      m_layout->addWidget(label, row_id, 0);
      m_layout->addWidget(catcher, row_id, 1);
      second_column = true;
    }
  }

  // Make sure that "spacer" is added.
  m_layout->setRowStretch(row_id, 1);
}
