#include "gui/basetoolbar.h"

#include "definitions/definitions.h"
#include "gui/formmain.h"
#include "miscellaneous/settings.h"


BaseToolBar::BaseToolBar(const QString &title, QWidget *parent)
  : QToolBar(title, parent) {
}

BaseToolBar::~BaseToolBar() {
  qDebug("Destroying BaseToolBar instance.");
}

void BaseToolBar::loadChangeableActions() {
  QHash<QString, QAction*> available_actions = FormMain::instance()->allActions();
  QStringList action_names = Settings::instance()->value(APP_CFG_GUI,
                                                         "messages_toolbar",
                                                         "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages").toString().split(',',
                                                                                                                                                                                                QString::SkipEmptyParts);

  actions().clear();

  // Iterate action names and add respectable actions into the toolbar.
  foreach (const QString &action_name, action_names) {
    if (available_actions.contains(action_name)) {
      addAction(available_actions.value(action_name));
    }
  }
}
