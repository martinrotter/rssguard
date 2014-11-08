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

#include "gui/feedstoolbar.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"


FeedsToolBar::FeedsToolBar(const QString &title, QWidget *parent) : BaseToolBar(title, parent) {
  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();
  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

FeedsToolBar::~FeedsToolBar() {
}

QHash<QString, QAction*> FeedsToolBar::availableActions() const {
  QList<QAction*> application_actions = qApp->userActions();
  QHash<QString, QAction*> available_actions;

  foreach (QAction *application_action, application_actions) {
    available_actions.insert(application_action->objectName(), application_action);
  }

  return available_actions;
}

QList<QAction*> FeedsToolBar::changeableActions() const {
  return actions();
}

void FeedsToolBar::saveChangeableActions(const QStringList &actions) {
  qApp->settings()->setValue(GROUP(GUI), "feeds_toolbar", actions.join(","));
  loadChangeableActions(actions);
}

void FeedsToolBar::loadChangeableActions() {
  QStringList action_names = qApp->settings()->value(GROUP(GUI),
                                                     "feeds_toolbar",
                                                     "m_actionUpdateAllFeeds,m_actionMarkAllFeedsRead").toString().split(',',
                                                                                                                         QString::SkipEmptyParts);

  loadChangeableActions(action_names);
}

void FeedsToolBar::loadChangeableActions(const QStringList &actions) {
  QHash<QString, QAction*> available_actions = availableActions();

  clear();

  // Iterate action names and add respectable actions into the toolbar.
  foreach (const QString &action_name, actions) {
    if (available_actions.contains(action_name)) {
      // Add existing standard action.
      addAction(available_actions.value(action_name));
    }
    else if (action_name == SEPARATOR_ACTION_NAME) {
      // Add new separator.
      addSeparator();
    }
    else if (action_name == SPACER_ACTION_NAME) {
      // Add new spacer.
      QWidget *spacer = new QWidget(this);
      spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      QAction *action = addWidget(spacer);
      action->setIcon(qApp->icons()->fromTheme("application-search"));
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("Toolbar spacer"));
    }
  }
}
