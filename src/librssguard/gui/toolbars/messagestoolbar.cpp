// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/toolbars/messagestoolbar.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "gui/reusable/nonclosablemenu.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>
#include <chrono>

using namespace std::chrono_literals;

MessagesToolBar::MessagesToolBar(const QString& title, QWidget* parent) : BaseToolBar(title, parent) {
  initializeSearchBox();
  initializeHighlighter();
}

QList<QAction*> MessagesToolBar::availableActions() const {
  QList<QAction*> available_actions = qApp->userActions();

  available_actions.append(m_actionSearchMessages);
  available_actions.append(m_actionMessageHighlighter);
  available_actions.append(m_actionMessageFilter);

  return available_actions;
}

QList<QAction*> MessagesToolBar::activatedActions() const {
  return actions();
}

void MessagesToolBar::saveAndSetActions(const QStringList& actions) {
  qApp->settings()->setValue(GROUP(GUI), GUI::MessagesToolbarDefaultButtons, actions.join(QSL(",")));
  loadSpecificActions(convertActions(actions));

  // If user hidden search messages box, then remove the filter.
  if (!activatedActions().contains(m_actionSearchMessages)) {
    m_txtSearchMessages->clear();
  }
}

QList<QAction*> MessagesToolBar::convertActions(const QStringList& actions) {
  QList<QAction*> available_actions = availableActions();
  QList<QAction*> spec_actions;

  // Iterate action names and add respectable actions into the toolbar.
  for (const QString& action_name : actions) {
    auto* matching_action = findMatchingAction(action_name, available_actions);

    if (matching_action != nullptr) {
      // Add existing standard action.
      spec_actions.append(matching_action);
    }
    else if (action_name == QSL(SEPARATOR_ACTION_NAME)) {
      // Add new separator.
      auto* act = new QAction(this);

      act->setSeparator(true);
      spec_actions.append(act);
    }
    else if (action_name == QSL(SEARCH_BOX_ACTION_NAME)) {
      // Add search box.
      spec_actions.append(m_actionSearchMessages);
    }
    else if (action_name.startsWith(QSL(HIGHLIGHTER_ACTION_NAME))) {
      // Add highlighter button.
      spec_actions.append(m_actionMessageHighlighter);
      activateAction(action_name, m_actionMessageHighlighter);
    }
    else if (action_name.startsWith(QSL(FILTER_ACTION_NAME))) {
      // Add filter button.
      spec_actions.append(m_actionMessageFilter);
      activateAction(action_name, m_actionMessageFilter);
    }
    else if (action_name == QSL(SPACER_ACTION_NAME)) {
      // Add new spacer.
      auto* spacer = new QWidget(this);

      spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      auto* action = new QWidgetAction(this);

      action->setDefaultWidget(spacer);
      action->setIcon(qApp->icons()->fromTheme(QSL("go-jump")));
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("Toolbar spacer"));
      spec_actions.append(action);
    }
  }

  return spec_actions;
}

void MessagesToolBar::loadSpecificActions(const QList<QAction*>& actions, bool initial_load) {
  Q_UNUSED(initial_load)

  clear();

  for (QAction* act : actions) {
    addAction(act);
  }
}

inline MessagesProxyModel::MessageListFilter operator|(MessagesProxyModel::MessageListFilter a,
                                                       MessagesProxyModel::MessageListFilter b) {
  return static_cast<MessagesProxyModel::MessageListFilter>(static_cast<int>(a) | static_cast<int>(b));
}

inline MessagesModel::MessageHighlighter operator|(MessagesModel::MessageHighlighter a,
                                                   MessagesModel::MessageHighlighter b) {
  return static_cast<MessagesModel::MessageHighlighter>(static_cast<int>(a) | static_cast<int>(b));
}

void MessagesToolBar::handleMessageHighlighterChange(QAction* action) {
  MessagesModel::MessageHighlighter task = action->data().value<MessagesModel::MessageHighlighter>();
  std::list<QAction*> checked_tasks_std = boolinq::from(m_menuMessageHighlighter->actions())
                                            .where([](QAction* act) {
                                              return act->isChecked();
                                            })
                                            .toStdList();

  if (task == MessagesModel::MessageHighlighter::NoHighlighting || checked_tasks_std.empty()) {
    checked_tasks_std.clear();

    // Uncheck everything.
    m_menuMessageHighlighter->blockSignals(true);

    for (QAction* tsk : m_menuMessageHighlighter->actions()) {
      tsk->setChecked(false);
    }

    m_menuMessageHighlighter->blockSignals(false);
  }
  else {
    task = MessagesModel::MessageHighlighter(0);

    for (QAction* tsk : checked_tasks_std) {
      task = task | tsk->data().value<MessagesModel::MessageHighlighter>();
    }
  }

  m_btnMessageHighlighter->setDefaultAction(checked_tasks_std.empty() ? m_menuMessageHighlighter->actions().constFirst()
                                                                      : checked_tasks_std.front());
  saveToolButtonSelection(HIGHLIGHTER_ACTION_NAME, FROM_STD_LIST(QList<QAction*>, checked_tasks_std));
  emit messageHighlighterChanged(task);
}

void MessagesToolBar::handleMessageFilterChange(QAction* action) {
  MessagesProxyModel::MessageListFilter task = action->data().value<MessagesProxyModel::MessageListFilter>();
  std::list<QAction*> checked_tasks_std = boolinq::from(m_menuMessageFilter->actions())
                                            .where([](QAction* act) {
                                              return act->isChecked();
                                            })
                                            .toStdList();

  if (task == MessagesProxyModel::MessageListFilter::NoFiltering || checked_tasks_std.empty()) {
    checked_tasks_std.clear();

    // Uncheck everything.
    m_menuMessageFilter->blockSignals(true);

    for (QAction* tsk : m_menuMessageFilter->actions()) {
      tsk->setChecked(false);
    }

    m_menuMessageFilter->blockSignals(false);
  }
  else {
    task = MessagesProxyModel::MessageListFilter(0);

    for (QAction* tsk : checked_tasks_std) {
      task = task | tsk->data().value<MessagesProxyModel::MessageListFilter>();
    }
  }

  m_btnMessageFilter->setDefaultAction(checked_tasks_std.empty() ? m_menuMessageFilter->actions().constFirst()
                                                                 : checked_tasks_std.front());
  saveToolButtonSelection(FILTER_ACTION_NAME, FROM_STD_LIST(QList<QAction*>, checked_tasks_std));
  emit messageFilterChanged(task);
}

void MessagesToolBar::initializeSearchBox() {
  m_txtSearchMessages =
    new SearchLineEdit({SearchLineEdit::CustomSearchChoice(tr("Everywhere"), int(SearchFields::SearchAll)),
                        SearchLineEdit::CustomSearchChoice(tr("Titles only"), int(SearchFields::SearchTitleOnly))},
                       this);
  m_txtSearchMessages->setSizePolicy(QSizePolicy::Policy::Expanding,
                                     m_txtSearchMessages->sizePolicy().verticalPolicy());
  m_txtSearchMessages->setPlaceholderText(tr("Search articles"));

  // Setup wrapping action for search box.
  m_actionSearchMessages = new QWidgetAction(this);
  m_actionSearchMessages->setDefaultWidget(m_txtSearchMessages);
  m_actionSearchMessages->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  m_actionSearchMessages->setProperty("type", SEARCH_BOX_ACTION_NAME);
  m_actionSearchMessages->setProperty("name", tr("Article search box"));

  connect(m_txtSearchMessages, &SearchLineEdit::searchCriteriaChanged, this, &MessagesToolBar::searchCriteriaChanged);
}

void MessagesToolBar::addActionToMenu(QMenu* menu,
                                      const QIcon& icon,
                                      const QString& title,
                                      const QVariant& value,
                                      const QString& name) {
  QAction* action = menu->addAction(icon, title);

  action->setCheckable(true);
  action->setData(value);
  action->setObjectName(name);
}

void MessagesToolBar::initializeHighlighter() {
  m_menuMessageHighlighter = new NonClosableMenu(tr("Menu for highlighting articles"), this);

  addActionToMenu(m_menuMessageHighlighter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("No extra highlighting"),
                  QVariant::fromValue(MessagesModel::MessageHighlighter::NoHighlighting),
                  QSL("no_highlighting"));
  addActionToMenu(m_menuMessageHighlighter,
                  qApp->icons()->fromTheme(QSL("mail-mark-unread")),
                  tr("Highlight unread articles"),
                  QVariant::fromValue(MessagesModel::MessageHighlighter::HighlightUnread),
                  QSL("highlight_unread"));
  addActionToMenu(m_menuMessageHighlighter,
                  qApp->icons()->fromTheme(QSL("mail-mark-important")),
                  tr("Highlight important articles"),
                  QVariant::fromValue(MessagesModel::MessageHighlighter::HighlightImportant),
                  QSL("highlight_important"));

  m_menuMessageFilter = new NonClosableMenu(tr("Menu for filtering articles"), this);

  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("No extra filtering"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::NoFiltering),
                  QSL("no_filtering"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-unread")),
                  tr("Show unread articles"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowUnread),
                  QSL("show_unread"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-important")),
                  tr("Show important articles"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowImportant),
                  QSL("show_important"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show today's articles"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowToday),
                  QSL("show_today"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show yesterday's articles"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowYesterday),
                  QSL("show_yesterday"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show articles in last 24 hours"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowLast24Hours),
                  QSL("show_last24hours"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show articles in last 48 hours"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowLast48Hours),
                  QSL("show_last48hours"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show this week's articles"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowThisWeek),
                  QSL("show_this_week"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show last week's articles"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowLastWeek),
                  QSL("show_last_week"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-attachment")),
                  tr("Show articles with attachments"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowOnlyWithAttachments),
                  QSL("show_with_attachments"));
  addActionToMenu(m_menuMessageFilter,
                  MessagesModel::generateIconForScore(MSG_SCORE_MAX / 2.0),
                  tr("Show articles with some score"),
                  QVariant::fromValue(MessagesProxyModel::MessageListFilter::ShowOnlyWithScore),
                  QSL("show_with_score"));

  m_btnMessageHighlighter = new QToolButton(this);
  m_btnMessageHighlighter->setToolTip(tr("Display all articles"));
  m_btnMessageHighlighter->setMenu(m_menuMessageHighlighter);
  m_btnMessageHighlighter->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_btnMessageHighlighter->setIcon(qApp->icons()->fromTheme(QSL("mail-mark-read")));
  m_btnMessageHighlighter->setDefaultAction(m_menuMessageHighlighter->actions().constFirst());

  m_btnMessageFilter = new QToolButton(this);
  m_btnMessageFilter->setToolTip(tr("Display all articles"));
  m_btnMessageFilter->setMenu(m_menuMessageFilter);
  m_btnMessageFilter->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_btnMessageFilter->setIcon(qApp->icons()->fromTheme(QSL("mail-mark-read")));
  m_btnMessageFilter->setDefaultAction(m_menuMessageFilter->actions().constFirst());

  m_actionMessageHighlighter = new QWidgetAction(this);
  m_actionMessageHighlighter->setDefaultWidget(m_btnMessageHighlighter);
  m_actionMessageHighlighter->setIcon(m_btnMessageHighlighter->icon());
  m_actionMessageHighlighter->setProperty("type", HIGHLIGHTER_ACTION_NAME);
  m_actionMessageHighlighter->setProperty("name", tr("Article highlighter"));

  m_actionMessageFilter = new QWidgetAction(this);
  m_actionMessageFilter->setDefaultWidget(m_btnMessageFilter);
  m_actionMessageFilter->setIcon(m_btnMessageFilter->icon());
  m_actionMessageFilter->setProperty("type", FILTER_ACTION_NAME);
  m_actionMessageFilter->setProperty("name", tr("Article list filter"));

  connect(m_menuMessageHighlighter, &QMenu::triggered, this, &MessagesToolBar::handleMessageHighlighterChange);
  connect(m_menuMessageFilter, &QMenu::triggered, this, &MessagesToolBar::handleMessageFilterChange);

  connect(this, &MessagesToolBar::toolButtonStyleChanged, this, [=](Qt::ToolButtonStyle style) {
    m_btnMessageHighlighter->setToolButtonStyle(style);
    m_btnMessageFilter->setToolButtonStyle(style);
  });
}

void MessagesToolBar::saveToolButtonSelection(const QString& button_name, const QList<QAction*>& actions) const {
  QStringList action_names = savedActions();

  auto opts_list = boolinq::from(actions)
                     .select([](const QAction* act) {
                       return act->objectName();
                     })
                     .toStdList();
  QStringList opts = FROM_STD_LIST(QStringList, opts_list);

  for (QString& action_name : action_names) {
    if (action_name.startsWith(button_name)) {
      action_name = button_name + QSL("[%1]").arg(opts.join(QL1C(';')));
    }
  }

  qApp->settings()->setValue(GROUP(GUI), GUI::MessagesToolbarDefaultButtons, action_names.join(QSL(",")));
}

SearchLineEdit* MessagesToolBar::searchBox() const {
  return m_txtSearchMessages;
}

void MessagesToolBar::activateAction(const QString& action_name, QWidgetAction* widget_action) {
  const int start = action_name.indexOf('[');
  const int end = action_name.indexOf(']');

  if (start != -1 && end != -1 && end == action_name.length() - 1) {
    const QStringList menu_action_names = action_name.chopped(1).right(end - start - 1).split(QL1C(';'));
    auto tool_btn = qobject_cast<QToolButton*>(widget_action->defaultWidget());

    for (QAction* action : tool_btn->menu()->actions()) {
      if (menu_action_names.contains(action->objectName())) {
        // tool_btn->setDefaultAction(action);
        action->trigger();
      }
    }
  }
}

QStringList MessagesToolBar::defaultActions() const {
  return QString(GUI::MessagesToolbarDefaultButtonsDef)
    .split(QL1C(','),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
           Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
           QString::SplitBehavior::SkipEmptyParts);
#endif
}

QStringList MessagesToolBar::savedActions() const {
  return qApp->settings()
    ->value(GROUP(GUI), SETTING(GUI::MessagesToolbarDefaultButtons))
    .toString()
    .split(QL1C(','),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
           Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
           QString::SplitBehavior::SkipEmptyParts);
#endif
}
