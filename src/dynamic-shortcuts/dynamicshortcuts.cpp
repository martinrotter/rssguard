// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "dynamic-shortcuts/dynamicshortcuts.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QAction>


DynamicShortcuts::DynamicShortcuts() {
}

void DynamicShortcuts::save(const QList<QAction*> actions) {
  Settings *settings = qApp->settings();

  foreach (QAction *action, actions) {
    settings->setValue(GROUP(Keyboard), action->objectName(), action->shortcut().toString(QKeySequence::PortableText));
  }
}

void DynamicShortcuts::load(const QList<QAction*> actions) {
  Settings *settings = qApp->settings();

  foreach (QAction *action, actions) {
    QString shortcut_for_action = settings->value(GROUP(Keyboard),
                                                  action->objectName(),
                                                  action->shortcut().toString(QKeySequence::PortableText)).toString();
    action->setShortcut(QKeySequence::fromString(shortcut_for_action, QKeySequence::PortableText));
  }
}
