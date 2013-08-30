#include <QAction>

#include "core/dynamicshortcuts.h"
#include "core/settings.h"
#include "core/defs.h"


DynamicShortcuts::DynamicShortcuts() {
}

void DynamicShortcuts::save(const QList<QAction *> actions) {
  Settings *settings = Settings::getInstance();

  foreach (QAction *action, actions) {
    settings->setValue(APP_CFG_CUTS,
                       action->objectName(),
                       action->shortcut().toString(QKeySequence::NativeText));
  }
}

void DynamicShortcuts::load(const QList<QAction *> actions) {
  Settings *settings = Settings::getInstance();

  foreach (QAction *action, actions) {
    QString shortcut_for_action = settings->value(APP_CFG_CUTS,
                                                  action->objectName(),
                                                  action->shortcut().toString(QKeySequence::NativeText)).toString();
    action->setShortcut(QKeySequence::fromString(shortcut_for_action,
                                                 QKeySequence::NativeText));
  }
}
