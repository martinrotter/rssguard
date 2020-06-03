// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/dynamicshortcuts.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QAction>

void DynamicShortcuts::save(const QList<QAction*>& actions) {
  Settings* settings = qApp->settings();

  for (const QAction* action : actions) {
    settings->setValue(GROUP(Keyboard), action->objectName(), action->shortcut().toString(QKeySequence::PortableText));
  }
}

void DynamicShortcuts::load(const QList<QAction*>& actions) {
  Settings* settings = qApp->settings();

  for (QAction* action : actions) {
    QString shortcut_for_action = settings->value(GROUP(Keyboard),
                                                  action->objectName(),
                                                  action->shortcut().toString(QKeySequence::PortableText)).toString();

    action->setShortcut(QKeySequence::fromString(shortcut_for_action, QKeySequence::PortableText));
  }
}
