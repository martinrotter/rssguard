#include "gui/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/formmain.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>
#include <QToolButton>
#include <QMenu>


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

  m_menuFilterMessages = new QMenu(tr("Menu for highlighting messages"), this);
  m_menuFilterMessages->addAction(IconFactory::instance()->fromTheme("mail-mark-read"),
                                  tr("No extra highlighting"))->setData(QVariant::fromValue(MessagesModel::DisplayAll));
  m_menuFilterMessages->addAction(IconFactory::instance()->fromTheme("mail-mark-unread"),
                                  tr("Highlight unread messages"))->setData(QVariant::fromValue(MessagesModel::DisplayUnread));
  m_menuFilterMessages->addAction(IconFactory::instance()->fromTheme("mail-mark-favorite"),
                                  tr("Highlight important messages"))->setData(QVariant::fromValue(MessagesModel::DisplayImportant));

  m_btnFilterMessages = new QToolButton(this);
  m_btnFilterMessages->setToolTip(tr("Display all messages"));
  m_btnFilterMessages->setMenu(m_menuFilterMessages);
  m_btnFilterMessages->setPopupMode(QToolButton::MenuButtonPopup);
  m_btnFilterMessages->setIcon(IconFactory::instance()->fromTheme("mail-mark-read"));

  m_actionFilterMessages = new QWidgetAction(this);
  m_actionFilterMessages->setDefaultWidget(m_btnFilterMessages);
  m_actionFilterMessages->setProperty("type", FILTER_ACTION_NAME);
  m_actionFilterMessages->setProperty("name", tr("Message highlighter"));

  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();
  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);

  connect(m_txtSearchMessages, SIGNAL(textChanged(QString)),
          this, SIGNAL(messageSearchPatternChanged(QString)));
  connect(m_menuFilterMessages, SIGNAL(triggered(QAction*)),
          this, SLOT(handleMessageFilterChange(QAction*)));
}

MessagesToolBar::~MessagesToolBar() {
}

QHash<QString, QAction*> MessagesToolBar::availableActions() const {
  QHash<QString, QAction*> available_actions = FormMain::instance()->allActions();
  available_actions.insert(SEACRH_MESSAGES_ACTION_NAME, m_actionSearchMessages);
  available_actions.insert(FILTER_ACTION_NAME, m_actionFilterMessages);
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
    else if (action_name == FILTER_ACTION_NAME) {
      // Add filter button.
      addAction(m_actionFilterMessages);
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

void MessagesToolBar::handleMessageFilterChange(QAction *action) {
  m_btnFilterMessages->setIcon(action->icon());
  m_btnFilterMessages->setToolTip(action->text());

  emit messageFilterChanged(action->data().value<MessagesModel::DisplayFilter>());
}

void MessagesToolBar::loadChangeableActions() {
  QStringList action_names = Settings::instance()->value(APP_CFG_GUI,
                                                         "messages_toolbar",
                                                         "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages,spacer,search").toString().split(',',
                                                                                                                                                                                                              QString::SkipEmptyParts);

  loadChangeableActions(action_names);
}
