// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/dynamicshortcutswidget.h"

#include "core/defs.h"
#include "gui/shortcutcatcher.h"
#include "gui/shortcutbutton.h"

#include <QGridLayout>
#include <QAction>
#include <QLabel>
#include <QSpacerItem>
#include <QPalette>


DynamicShortcutsWidget::DynamicShortcutsWidget(QWidget *parent) : QWidget(parent) {
  // Create layout for this control and set is as active.
  m_layout = new QGridLayout(this);
  m_layout->setMargin(0);

  setLayout(m_layout);
}

DynamicShortcutsWidget::~DynamicShortcutsWidget() {
  delete m_layout;
}

bool DynamicShortcutsWidget::areShortcutsUnique() {
  QList<QKeySequence> all_shortcuts;

  // Obtain all shortcuts.
  foreach (const ActionBinding &binding, m_actionBindings) {
    QKeySequence new_shortcut = binding.second->shortcut();

    if (!new_shortcut.isEmpty() && all_shortcuts.contains(new_shortcut)) {
      // Problem, two identical non-empty shortcuts found.
      return false;
    }
    else {
      all_shortcuts.append(binding.second->shortcut());
    }
  }

  return true;
}

void DynamicShortcutsWidget::updateShortcuts() {
  foreach (const ActionBinding &binding, m_actionBindings) {
    binding.first->setShortcut(binding.second->shortcut());
  }
}

void DynamicShortcutsWidget::populate(const QList<QAction*> actions) {
  m_actionBindings.clear();

  int row_id = 0;

  foreach (QAction *action, actions) {
    // Create shortcut catcher for this action and set default shortcut.
    ShortcutCatcher *catcher = new ShortcutCatcher(this);
    catcher->setDefaultShortcut(action->shortcut());

    // Store information for re-initialization of shortcuts
    // of actions when widget gets "confirmed".
    QPair<QAction*,ShortcutCatcher*> new_binding;
    new_binding.first = action;
    new_binding.second = catcher;

    m_actionBindings << new_binding;

    // Add new catcher to our control.
    QLabel *action_label = new QLabel(this);
    action_label->setText(action->text().remove('&'));
    action_label->setToolTip(action->toolTip());
    action_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QLabel *action_icon = new QLabel(this);
    action_icon->setPixmap(action->icon().pixmap(ICON_SIZE_SETTINGS, ICON_SIZE_SETTINGS));
    action_icon->setToolTip(action->toolTip());

    m_layout->addWidget(action_icon, row_id, 0);
    m_layout->addWidget(action_label, row_id, 1);
    m_layout->addWidget(catcher, row_id, 2);

    row_id++;
  }

  // Make sure that "spacer" is added.
  m_layout->setRowStretch(row_id, 1);
  m_layout->setColumnStretch(1, 1);
}
