// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/basetoolbar.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>

BaseToolBar::BaseToolBar(const QString& title, QWidget* parent) : QToolBar(title, parent) {
  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();

  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

BaseToolBar::~BaseToolBar() {
  qDebug("Destroying BaseToolBar instance.");
}

void BaseBar::loadSavedActions() {
  loadSpecificActions(getSpecificActions(savedActions()));
}

QAction* BaseBar::findMatchingAction(const QString& action, const QList<QAction*>& actions) const {
  foreach (QAction* act, actions) {
    if (act->objectName() == action) {
      return act;
    }
  }

  return nullptr;
}
