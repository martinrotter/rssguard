#include "gui/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/formmain.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>


MessagesToolBar::MessagesToolBar(const QString &title, QWidget *parent)
  : BaseToolBar(title, parent),
    m_txtSearchMessages(new BaseLineEdit(this)) {

  m_txtSearchMessages->setFixedWidth(FILTER_WIDTH);
  m_txtSearchMessages->setPlaceholderText(tr("Search messages"));

  // Setup wrapping action for search box.
  m_actionSearchMessages = new QWidgetAction(this);
  m_actionSearchMessages->setDefaultWidget(m_txtSearchMessages);
  m_actionSearchMessages->setIcon(IconFactory::instance()->fromTheme("view-spacer"));
  m_actionSearchMessages->setProperty("type", SEACRH_MESSAGES_ACTION_NAME);
  m_actionSearchMessages->setProperty("name", tr("Message search box"));

  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();
  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);

  connect(m_txtSearchMessages, SIGNAL(textChanged(QString)),
          this, SIGNAL(messageSearchPatternChanged(QString)));
}

MessagesToolBar::~MessagesToolBar() {
}

QHash<QString, QAction*> MessagesToolBar::availableActions() const {
  QHash<QString, QAction*> available_actions = FormMain::instance()->allActions();
  available_actions.insert(SEACRH_MESSAGES_ACTION_NAME, m_actionSearchMessages);
  return available_actions;
}

QList<QAction*> MessagesToolBar::changeableActions() const {
  return actions();
}

void MessagesToolBar::saveChangeableActions(const QStringList& actions) {
  Settings::instance()->setValue(APP_CFG_GUI, "messages_toolbar", actions.join(","));
  loadChangeableActions(actions);

  // If user hidden search messages box, then remove the filter.
  if (!changeableActions().contains(m_actionSearchMessages)) {
    m_txtSearchMessages->clear();
  }
}

void MessagesToolBar::loadChangeableActions(const QStringList& actions) {
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
    else if (action_name == SEACRH_MESSAGES_ACTION_NAME) {
      // Add search box.
      addAction(m_actionSearchMessages);
    }
    else if (action_name == SPACER_ACTION_NAME) {
      // Add new spacer.
      QWidget *spacer = new QWidget(this);
      spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      QAction *action = addWidget(spacer);
      action->setIcon(IconFactory::instance()->fromTheme("application-search"));
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("Toolbar spacer"));
    }
  }
}

void MessagesToolBar::loadChangeableActions() {
  QStringList action_names = Settings::instance()->value(APP_CFG_GUI,
                                                         "messages_toolbar",
                                                         "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages,spacer,filter").toString().split(',',
                                                                                                                                                                                                              QString::SkipEmptyParts);

  loadChangeableActions(action_names);
}
