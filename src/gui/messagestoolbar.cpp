#include "gui/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/formmain.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>


MessagesToolBar::MessagesToolBar(const QString &title, QWidget *parent)
  : BaseToolBar(title, parent),
    m_txtFilter(new BaseLineEdit(this)) {
  m_txtFilter->setFixedWidth(FILTER_WIDTH);
  m_txtFilter->setPlaceholderText(tr("Filter messages"));
  m_actionFilter = new QWidgetAction(this);
  m_actionFilter->setDefaultWidget(m_txtFilter);
  m_actionFilter->setProperty("type", FILTER_OBJECT_NAME);
  m_actionFilter->setProperty("name", tr("message filter"));

  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();
  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

MessagesToolBar::~MessagesToolBar() {
}

QList<QAction*> MessagesToolBar::availableActions() const {
  QList<QAction*> available_actions = FormMain::instance()->allActions().values();
  available_actions.append(m_actionFilter);
  return available_actions;
}

QList<QAction*> MessagesToolBar::changeableActions() const {
  return actions();
}

void MessagesToolBar::saveChangeableActions() const {
  QStringList action_names;

  // Iterates all actions present in the toolbar and
  // returns actions which can be replaced by user.
  foreach (QAction *action, actions()) {
    if (action->isSeparator()) {
      // This action is separator, add its "name" to settings.
      action_names.append(SEPARATOR_ACTION_NAME);
    }
    else if (action->property("type").isValid()) {
      // This action is extra widget or spacer.
      action_names.append(action->property("type").toString());
    }
    else {
      // This action is normal action.
      action_names.append(action->objectName());
    }
  }

  Settings::instance()->setValue(APP_CFG_GUI, "messages_toolbar", action_names.join(","));
}

void MessagesToolBar::saveChangeableActions(const QStringList& actions) {
  Settings::instance()->setValue(APP_CFG_GUI, "messages_toolbar", actions.join(","));
  loadChangeableActions();
}

void MessagesToolBar::loadChangeableActions() {
  QHash<QString, QAction*> available_actions = FormMain::instance()->allActions();
  QStringList action_names = Settings::instance()->value(APP_CFG_GUI,
                                                         "messages_toolbar",
                                                         "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages,spacer,filter").toString().split(',',
                                                                                                                                                                                                              QString::SkipEmptyParts);

  clear();

  // Iterate action names and add respectable actions into the toolbar.
  foreach (const QString &action_name, action_names) {
    if (available_actions.contains(action_name)) {
      // Add existing standard action.
      addAction(available_actions.value(action_name));
    }
    else if (action_name == SEPARATOR_ACTION_NAME) {
      // Add new separator.
      addSeparator();
    }
    else if (action_name == FILTER_OBJECT_NAME) {
      // Add filter.
      addAction(m_actionFilter);
    }
    else if (action_name == SPACER_ACTION_NAME) {
      // Add new spacer.
      QWidget *spacer = new QWidget(this);
      spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      QAction *action = addWidget(spacer);
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("spacer"));
    }
  }
}
