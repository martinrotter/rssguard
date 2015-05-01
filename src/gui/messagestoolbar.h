// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "gui/basetoolbar.h"

#include "core/messagesmodel.h"


class MessagesSearchLineEdit;
class QWidgetAction;
class QToolButton;
class QMenu;

class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesToolBar(const QString &title, QWidget *parent = 0);
    virtual ~MessagesToolBar();

    // External access to search line edit.
    inline MessagesSearchLineEdit *searchLineEdit() {
      return m_txtSearchMessages;
    }

    // Implementation of BaseToolBar interface.
    QList<QAction*> availableActions() const;
    QList<QAction*> changeableActions() const;
    void saveChangeableActions(const QStringList &actions);
    void loadChangeableActions();

    // Loads actions as specified by external actions list.
    // NOTE: This is used primarily for reloading actions
    // when they are changed from settings.
    void loadChangeableActions(const QStringList &actions);

  signals:
    void messageSearchPatternChanged(const QString &pattern);

    // Emitted if message filter is changed.
    void messageFilterChanged(MessagesModel::MessageFilter filter);

  private slots:
    // Called when highlighter gets changed.
    void handleMessageHighlighterChange(QAction *action);

  private:
    void initializeSearchBox();
    void initializeHighlighter();

  private:
    QWidgetAction *m_actionMessageHighlighter;
    QToolButton *m_btnMessageHighlighter;
    QMenu *m_menuMessageHighlighter;

    QWidgetAction *m_actionSearchMessages;
    MessagesSearchLineEdit *m_txtSearchMessages;
};

#endif // NEWSTOOLBAR_H
