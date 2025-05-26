// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/toolbars/feedstoolbar.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/feedsproxymodel.h"
#include "gui/reusable/nonclosablemenu.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>

FeedsToolBar::FeedsToolBar(const QString& title, QWidget* parent) : BaseToolBar(title, parent) {
  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();

  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);

  initializeFilter();
  initializeSearchBox();
}

QList<QAction*> FeedsToolBar::availableActions() const {
  QList<QAction*> available_actions = qApp->userActions();

  available_actions.append(m_actionSearchMessages);
  available_actions.append(m_actionMessageFilter);

  return available_actions;
}

QList<QAction*> FeedsToolBar::activatedActions() const {
  return actions();
}

void FeedsToolBar::saveAndSetActions(const QStringList& actions) {
  qApp->settings()->setValue(GROUP(GUI), GUI::FeedsToolbarActions, actions.join(QSL(",")));
  loadSpecificActions(convertActions(actions));

  // If user hidden search messages box, then remove the filter.
  if (!activatedActions().contains(m_actionSearchMessages)) {
    m_txtSearchMessages->clear();
  }
}

QList<QAction*> FeedsToolBar::convertActions(const QStringList& actions) {
  QList<QAction*> available_actions = availableActions();
  QList<QAction*> spec_actions;

  spec_actions.reserve(actions.size());

  // Iterate action names and add respectable actions into the toolbar.
  for (const QString& action_name : actions) {
    QAction* matching_action = findMatchingAction(action_name, available_actions);

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
      action->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("Toolbar spacer"));
      spec_actions.append(action);
    }
  }

  return spec_actions;
}

void FeedsToolBar::loadSpecificActions(const QList<QAction*>& actions, bool initial_load) {
  Q_UNUSED(initial_load)

  clear();

  for (QAction* act : actions) {
    addAction(act);
  }
}

inline FeedsProxyModel::FeedListFilter operator|(FeedsProxyModel::FeedListFilter a, FeedsProxyModel::FeedListFilter b) {
  return static_cast<FeedsProxyModel::FeedListFilter>(static_cast<int>(a) | static_cast<int>(b));
}

void FeedsToolBar::handleMessageFilterChange(QAction* action) {
  FeedsProxyModel::FeedListFilter task = action->data().value<FeedsProxyModel::FeedListFilter>();
  std::list<QAction*> checked_tasks_std = boolinq::from(m_menuMessageFilter->actions())
                                            .where([](QAction* act) {
                                              return act->isChecked();
                                            })
                                            .toStdList();

  if (task == FeedsProxyModel::FeedListFilter::NoFiltering || checked_tasks_std.empty()) {
    task = FeedsProxyModel::FeedListFilter::NoFiltering;

    checked_tasks_std.clear();

    // Uncheck everything.
    m_menuMessageFilter->blockSignals(true);

    for (QAction* tsk : m_menuMessageFilter->actions()) {
      tsk->setChecked(false);
    }

    m_menuMessageFilter->blockSignals(false);
  }
  else {
    task = FeedsProxyModel::FeedListFilter(0);

    for (QAction* tsk : checked_tasks_std) {
      task = task | tsk->data().value<FeedsProxyModel::FeedListFilter>();
    }
  }

  m_btnMessageFilter->setDefaultAction(checked_tasks_std.empty() ? m_menuMessageFilter->actions().constFirst()
                                                                 : checked_tasks_std.front());

  if (checked_tasks_std.size() > 1) {
    drawNumberOfCriterias(m_btnMessageFilter, int(checked_tasks_std.size()));
  }

  saveToolButtonSelection(QSL(FILTER_ACTION_NAME),
                          GUI::FeedsToolbarActions,
                          FROM_STD_LIST(QList<QAction*>, checked_tasks_std));
  emit feedFilterChanged(task);
}

QStringList FeedsToolBar::defaultActions() const {
  return QString(GUI::FeedsToolbarActionsDef)
    .split(',',
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
           Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
           QString::SplitBehavior::SkipEmptyParts);
#endif
}

QStringList FeedsToolBar::savedActions() const {
  return qApp->settings()
    ->value(GROUP(GUI), SETTING(GUI::FeedsToolbarActions))
    .toString()
    .split(',',
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
           Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
           QString::SplitBehavior::SkipEmptyParts);
#endif
}

QList<QAction*> FeedsToolBar::extraActions() const {
  return m_menuMessageFilter->actions();
}

void FeedsToolBar::initializeSearchBox() {
  m_txtSearchMessages =
    new SearchLineEdit(QSL("feed_list_searcher"),
                       {SearchLineEdit::CustomSearchChoice(tr("Everywhere"), int(SearchFields::SearchAll)),
                        SearchLineEdit::CustomSearchChoice(tr("Titles only"), int(SearchFields::SearchTitleOnly))},
                       this);
  m_txtSearchMessages->setSizePolicy(QSizePolicy::Policy::Expanding,
                                     m_txtSearchMessages->sizePolicy().verticalPolicy());
  m_txtSearchMessages->setPlaceholderText(tr("Search feeds"));

  // Setup wrapping action for search box.
  m_actionSearchMessages = new QWidgetAction(this);

  m_actionSearchMessages->setDefaultWidget(m_txtSearchMessages);
  m_actionSearchMessages->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  m_actionSearchMessages->setProperty("type", SEARCH_BOX_ACTION_NAME);
  m_actionSearchMessages->setProperty("name", tr("Feeds search box"));

  connect(m_txtSearchMessages, &SearchLineEdit::searchCriteriaChanged, this, &FeedsToolBar::searchCriteriaChanged);
}

SearchLineEdit* FeedsToolBar::searchBox() const {
  return m_txtSearchMessages;
}

void FeedsToolBar::initializeFilter() {
  m_menuMessageFilter = new NonClosableMenu(tr("Menu for filtering feeds"), this);

  QString fl = QSL(" ") + tr("(feed list)");

  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("No extra filtering"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::NoFiltering),
                  QSL("feedlist_no_filtering"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show unread items"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowUnread),
                  QSL("feedlist_show_unread"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show non-empty items"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowNonEmpty),
                  QSL("feedlist_non_empty"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show feeds with new articles"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowWithNewArticles),
                  QSL("feedlist_new_articles"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show feeds with error"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowWithError),
                  QSL("feedlist_with_error"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show switched off feeds"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowSwitchedOff),
                  QSL("feedlist_switched_off"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show quiet feeds"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowQuiet),
                  QSL("feedlist_quiet"));
  addActionToMenu(m_menuMessageFilter,
                  qApp->icons()->fromTheme(QSL("mail-mark-read")),
                  tr("Show feeds with article filters"),
                  fl,
                  QVariant::fromValue(FeedsProxyModel::FeedListFilter::ShowWithArticleFilters),
                  QSL("feedlist_with_filters"));

  m_btnMessageFilter = new QToolButton(this);
  m_btnMessageFilter->setToolTip(tr("Display all feeds"));
  m_btnMessageFilter->setMenu(m_menuMessageFilter);
  m_btnMessageFilter->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_btnMessageFilter->setIcon(qApp->icons()->fromTheme(QSL("mail-mark-read")));
  m_btnMessageFilter->setDefaultAction(m_menuMessageFilter->actions().constFirst());

  m_actionMessageFilter = new QWidgetAction(this);
  m_actionMessageFilter->setDefaultWidget(m_btnMessageFilter);
  m_actionMessageFilter->setIcon(m_btnMessageFilter->icon());
  m_actionMessageFilter->setProperty("type", FILTER_ACTION_NAME);
  m_actionMessageFilter->setProperty("name", tr("Feed list filter"));

  connect(m_menuMessageFilter, &QMenu::triggered, this, &FeedsToolBar::handleMessageFilterChange);
  connect(this, &FeedsToolBar::toolButtonStyleChanged, this, [=](Qt::ToolButtonStyle style) {
    m_btnMessageFilter->setToolButtonStyle(style);
  });
}
