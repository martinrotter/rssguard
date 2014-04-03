#include "gui/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/formmain.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>


MessagesToolBar::MessagesToolBar(const QString &title, QWidget *parent)
  : BaseToolBar(title, parent),
    m_spacer(new QWidget(this)),
    m_txtFilter(new BaseLineEdit(this)) {
  m_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_txtFilter->setFixedWidth(FILTER_WIDTH);
  m_txtFilter->setPlaceholderText(tr("Filter messages"));

  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();
  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

MessagesToolBar::~MessagesToolBar() {
}

QList<QAction*> MessagesToolBar::changeableActions() const {
  // TODO: Vracet akce, ktere muze uzivatel upravovat v tomto toolbaru.
  // nebudou se tedy vracet spacer widgety nebo lineedity a tak podobně,
  // proste jen akce ktere sou uzivatelsky upravitelne
  // http://stackoverflow.com/questions/5364957/in-qt-4-7-how-can-a-pop-up-menu-be-added-to-a-qtoolbar-button
  QList<QAction*> changeable_actions;

  // Iterates all actions present in the toolbar and
  // returns actions which can be replaced by user.
  foreach (QAction *action, actions()) {
    QString action_name = action->objectName();

    if (action_name != FILTER_OBJECT_NAME && action_name != SPACER_OBJECT_NAME) {
      changeable_actions.append(action);
    }
  }

  return changeable_actions;
}

void MessagesToolBar::saveChangeableActions() const {
  QStringList action_names;

  // Iterates all actions present in the toolbar and
  // returns actions which can be replaced by user.
  foreach (QAction *action, actions()) {
    QString action_name = action->objectName();

    if (action_name != FILTER_OBJECT_NAME && action_name != SPACER_OBJECT_NAME) {
      action_names.append(action->objectName());
    }
    else if (action->isSeparator()) {
      action_names.append(SEPARATOR_ACTION_NAME);
    }
  }

  Settings::instance()->setValue(APP_CFG_GUI, "messages_toolbar", action_names.join(","));
}

void MessagesToolBar::loadChangeableActions() {
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
    else if (action_name == SEPARATOR_ACTION_NAME) {
      addSeparator();
    }
  }

  addWidget(m_spacer)->setObjectName(SPACER_OBJECT_NAME);
  addWidget(m_txtFilter)->setObjectName(FILTER_OBJECT_NAME);
}
