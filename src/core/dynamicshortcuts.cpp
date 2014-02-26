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

#include "core/dynamicshortcuts.h"

#include "core/defs.h"
#include "core/settings.h"

#include <QAction>


DynamicShortcuts::DynamicShortcuts() {
}

void DynamicShortcuts::save(const QList<QAction *> actions) {
  Settings *settings = Settings::instance();

  foreach (QAction *action, actions) {
    settings->setValue(APP_CFG_CUTS,
                       action->objectName(),
                       action->shortcut().toString(QKeySequence::NativeText));
  }
}

void DynamicShortcuts::load(const QList<QAction *> actions) {
  Settings *settings = Settings::instance();

  foreach (QAction *action, actions) {
    QString shortcut_for_action = settings->value(APP_CFG_CUTS,
                                                  action->objectName(),
                                                  action->shortcut().toString(QKeySequence::NativeText)).toString();
    action->setShortcut(QKeySequence::fromString(shortcut_for_action,
                                                 QKeySequence::NativeText));
  }
}
