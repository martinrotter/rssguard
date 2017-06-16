// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef TOOLBAREDITOR_H
#define TOOLBAREDITOR_H

#include <QWidget>

#include "ui_toolbareditor.h"


namespace Ui {
  class ToolBarEditor;
}

class BaseBar;

class ToolBarEditor : public QWidget {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit ToolBarEditor(QWidget *parent = 0);
    virtual ~ToolBarEditor();

    // Toolbar operations.
    void loadFromToolBar(BaseBar *tool_bar);
    void saveToolBar();

    inline QListWidget *activeItemsWidget() const {
      return m_ui->m_listActivatedActions;
    }

    inline QListWidget *availableItemsWidget() const {
      return m_ui->m_listAvailableActions;
    }

  protected:
    bool eventFilter(QObject *object, QEvent *event);

  private slots:
    void updateActionsAvailability();

    // Insert common controls.
    void insertSpacer();
    void insertSeparator();

    void moveActionDown();
    void moveActionUp();

    void addSelectedAction();
    void deleteSelectedAction();
    void deleteAllActions();

    void resetToolBar();

  signals:
    void setupChanged();

  private:
    void loadEditor(const QList<QAction*> activated_actions, const QList<QAction*> available_actions);

    QScopedPointer<Ui::ToolBarEditor> m_ui;
    BaseBar *m_toolBar;
};

#endif // TOOLBAREDITOR_H
