// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/messagessearchlineedit.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>
#include <QToolButton>
#include <QMenu>


MessagesToolBar::MessagesToolBar(const QString &title, QWidget *parent)
  : BaseToolBar(title, parent) {
  initializeSearchBox();
  initializeHighlighter();
}

MessagesToolBar::~MessagesToolBar() {
}

QList<QAction*> MessagesToolBar::availableActions() const {
  QList<QAction*> available_actions = qApp->userActions();

  available_actions.append(m_actionSearchMessages);
  available_actions.append(m_actionMessageHighlighter);

  return available_actions;
}

QList<QAction*> MessagesToolBar::changeableActions() const {
  return actions();
}

void MessagesToolBar::saveChangeableActions(const QStringList& actions) {
  qApp->settings()->setValue(GROUP(GUI), GUI::MessagesToolbarDefaultButtons, actions.join(QSL(",")));
  loadChangeableActions(actions);

  // If user hidden search messages box, then remove the filter.
  if (!changeableActions().contains(m_actionSearchMessages)) {
    m_txtSearchMessages->clear();
  }
}

void MessagesToolBar::loadChangeableActions(const QStringList& actions) {
  QList<QAction*> available_actions = availableActions();

  clear();

  // Iterate action names and add respectable actions into the toolbar.
  foreach (const QString &action_name, actions) {
    QAction *matching_action = findMatchingAction(action_name, available_actions);

    if (matching_action != nullptr) {
      // Add existing standard action.
      addAction(matching_action);
    }
    else if (action_name == SEPARATOR_ACTION_NAME) {
      // Add new separator.
      addSeparator();
    }
    else if (action_name == SEACRH_MESSAGES_ACTION_NAME) {
      // Add search box.
      addAction(m_actionSearchMessages);
    }
    else if (action_name == HIGHLIGHTER_ACTION_NAME) {
      // Add filter button.
      addAction(m_actionMessageHighlighter);
    }
    else if (action_name == SPACER_ACTION_NAME) {
      // Add new spacer.
      QWidget *spacer = new QWidget(this);
      spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      QAction *action = addWidget(spacer);
      action->setIcon(qApp->icons()->fromTheme(QSL("go-jump")));
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("Toolbar spacer"));
    }
  }
}

void MessagesToolBar::handleMessageHighlighterChange(QAction *action) {
  m_btnMessageHighlighter->setIcon(action->icon());
  m_btnMessageHighlighter->setToolTip(action->text());

  emit messageFilterChanged(action->data().value<MessagesModel::MessageHighlighter>());
}

void MessagesToolBar::initializeSearchBox() {
  m_txtSearchMessages = new MessagesSearchLineEdit(this);
  m_txtSearchMessages->setFixedWidth(FILTER_WIDTH);
  m_txtSearchMessages->setPlaceholderText(tr("Search messages"));

  // Setup wrapping action for search box.
  m_actionSearchMessages = new QWidgetAction(this);
  m_actionSearchMessages->setDefaultWidget(m_txtSearchMessages);
  m_actionSearchMessages->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  m_actionSearchMessages->setProperty("type", SEACRH_MESSAGES_ACTION_NAME);
  m_actionSearchMessages->setProperty("name", tr("Message search box"));

  connect(m_txtSearchMessages, SIGNAL(textChanged(QString)),
          this, SIGNAL(messageSearchPatternChanged(QString)));
}

void MessagesToolBar::initializeHighlighter() {
  m_menuMessageHighlighter = new QMenu(tr("Menu for highlighting messages"), this);
  m_menuMessageHighlighter->addAction(qApp->icons()->fromTheme(QSL("mail-mark-read")),
                                      tr("No extra highlighting"))->setData(QVariant::fromValue(MessagesModel::NoHighlighting));
  m_menuMessageHighlighter->addAction(qApp->icons()->fromTheme(QSL("mail-mark-unread")),
                                      tr("Highlight unread messages"))->setData(QVariant::fromValue(MessagesModel::HighlightUnread));
  m_menuMessageHighlighter->addAction(qApp->icons()->fromTheme(QSL("mail-mark-important")),
                                      tr("Highlight important messages"))->setData(QVariant::fromValue(MessagesModel::HighlightImportant));

  m_btnMessageHighlighter = new QToolButton(this);
  m_btnMessageHighlighter->setToolTip(tr("Display all messages"));
  m_btnMessageHighlighter->setMenu(m_menuMessageHighlighter);
  m_btnMessageHighlighter->setPopupMode(QToolButton::MenuButtonPopup);
  m_btnMessageHighlighter->setIcon(qApp->icons()->fromTheme(QSL("mail-mark-read")));

  m_actionMessageHighlighter = new QWidgetAction(this);
  m_actionMessageHighlighter->setDefaultWidget(m_btnMessageHighlighter);
  m_actionMessageHighlighter->setIcon(m_btnMessageHighlighter->icon());
  m_actionMessageHighlighter->setProperty("type", HIGHLIGHTER_ACTION_NAME);
  m_actionMessageHighlighter->setProperty("name", tr("Message highlighter"));

  connect(m_menuMessageHighlighter, SIGNAL(triggered(QAction*)),
          this, SLOT(handleMessageHighlighterChange(QAction*)));
}

void MessagesToolBar::loadChangeableActions() {
  QStringList action_names = qApp->settings()->value(GROUP(GUI),
                                                     SETTING(GUI::MessagesToolbarDefaultButtons)).toString().split(',',
                                                                                                                   QString::SkipEmptyParts);

  loadChangeableActions(action_names);
}
