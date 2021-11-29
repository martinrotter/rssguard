// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/toolbars/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/reusable/baselineedit.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <chrono>
#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>

using namespace std::chrono_literals;

MessagesToolBar::MessagesToolBar(const QString& title, QWidget* parent) : BaseToolBar(title, parent) {
  initializeSearchBox();
  initializeHighlighter();
}

QList<QAction*> MessagesToolBar::availableActions() const {
  QList<QAction*> available_actions = qApp->userActions();

  available_actions.append(m_actionSearchMessages);
  available_actions.append(m_actionMessageHighlighter);

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
    else if (action_name == QSL(HIGHLIGHTER_ACTION_NAME)) {
      // Add filter button.
      spec_actions.append(m_actionMessageHighlighter);
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

void MessagesToolBar::handleMessageHighlighterChange(QAction* action) {
  m_btnMessageHighlighter->setIcon(action->icon());
  m_btnMessageHighlighter->setToolTip(action->text());
  emit messageFilterChanged(action->data().value<MessagesModel::MessageHighlighter>());
}

void MessagesToolBar::initializeSearchBox() {
  m_tmrSearchPattern = new QTimer(this);
  m_tmrSearchPattern->setSingleShot(true);

  m_txtSearchMessages = new BaseLineEdit(this);
  m_txtSearchMessages->setSizePolicy(QSizePolicy::Policy::Expanding, m_txtSearchMessages->sizePolicy().verticalPolicy());
  m_txtSearchMessages->setPlaceholderText(tr("Search articles (regex only)"));

  // Setup wrapping action for search box.
  m_actionSearchMessages = new QWidgetAction(this);
  m_actionSearchMessages->setDefaultWidget(m_txtSearchMessages);
  m_actionSearchMessages->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  m_actionSearchMessages->setProperty("type", SEARCH_BOX_ACTION_NAME);
  m_actionSearchMessages->setProperty("name", tr("Article search box"));

  connect(m_txtSearchMessages, &BaseLineEdit::textChanged, this, &MessagesToolBar::onSearchPatternChanged);
  connect(m_tmrSearchPattern, &QTimer::timeout, this, [this]() {
    emit messageSearchPatternChanged(m_searchPattern);
  });
}

void MessagesToolBar::initializeHighlighter() {
  m_menuMessageHighlighter = new QMenu(tr("Menu for highlighting articles"), this);
  m_menuMessageHighlighter->addAction(qApp->icons()->fromTheme(QSL("mail-mark-read")),
                                      tr("No extra highlighting"))->setData(QVariant::fromValue(MessagesModel::MessageHighlighter::NoHighlighting));
  m_menuMessageHighlighter->addAction(qApp->icons()->fromTheme(QSL("mail-mark-unread")),
                                      tr("Highlight unread articles"))->setData(QVariant::fromValue(MessagesModel::MessageHighlighter::HighlightUnread));
  m_menuMessageHighlighter->addAction(qApp->icons()->fromTheme(QSL("mail-mark-important")),
                                      tr("Highlight important articles"))->setData(QVariant::fromValue(MessagesModel::MessageHighlighter::HighlightImportant));
  m_btnMessageHighlighter = new QToolButton(this);
  m_btnMessageHighlighter->setToolTip(tr("Display all articles"));
  m_btnMessageHighlighter->setMenu(m_menuMessageHighlighter);
  m_btnMessageHighlighter->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
  m_btnMessageHighlighter->setIcon(qApp->icons()->fromTheme(QSL("mail-mark-read")));
  m_actionMessageHighlighter = new QWidgetAction(this);
  m_actionMessageHighlighter->setDefaultWidget(m_btnMessageHighlighter);
  m_actionMessageHighlighter->setIcon(m_btnMessageHighlighter->icon());
  m_actionMessageHighlighter->setProperty("type", HIGHLIGHTER_ACTION_NAME);
  m_actionMessageHighlighter->setProperty("name", tr("Article highlighter"));

  connect(m_menuMessageHighlighter, &QMenu::triggered, this, &MessagesToolBar::handleMessageHighlighterChange);
}

QStringList MessagesToolBar::defaultActions() const {
  return QString(GUI::MessagesToolbarDefaultButtonsDef).split(QL1C(','),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                              Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                              QString::SplitBehavior::SkipEmptyParts);
#endif
}

QStringList MessagesToolBar::savedActions() const {
  return qApp->settings()->value(GROUP(GUI),
                                 SETTING(GUI::MessagesToolbarDefaultButtons)).toString().split(QL1C(','),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                                                               Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                                                               QString::SplitBehavior::SkipEmptyParts);
#endif
}

void MessagesToolBar::onSearchPatternChanged(const QString& search_pattern) {
  m_searchPattern = search_pattern;
  m_tmrSearchPattern->start(search_pattern.isEmpty() ? 0ms : 300ms);
}
