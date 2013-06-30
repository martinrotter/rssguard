#include <QAction>

#include "core/dynamicshortcuts.h"
#include "core/settings.h"
#include "core/defs.h"


DynamicShortcuts::DynamicShortcuts() {
}

void DynamicShortcuts::save(const QList<QAction *> actions) {
  foreach (QAction *action, actions) {
    Settings::getInstance()->setValue(APP_CFG_CUTS,
                                      action->objectName(),
                                      action->shortcut().toString(QKeySequence::NativeText));
  }
}

void DynamicShortcuts::load(const QList<QAction *> actions) {
  foreach (QAction *action, actions) {
    QString shortcut_for_action = Settings::getInstance()->value(APP_CFG_CUTS,
                                                                 action->objectName(),
                                                                 action->shortcut().toString(QKeySequence::NativeText)).toString();
    action->setShortcut(QKeySequence::fromString(shortcut_for_action,
                                                 QKeySequence::NativeText));
  }
}
