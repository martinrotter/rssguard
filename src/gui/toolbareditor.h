// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

class BaseToolBar;

class ToolBarEditor : public QWidget {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit ToolBarEditor(QWidget *parent = 0);
    virtual ~ToolBarEditor();

    // Toolbar operations.
    void loadFromToolBar(BaseToolBar *tool_bar);
    void saveToolBar();

    inline QListWidget *activeItemsWidget() {
      return m_ui->m_listActivatedActions;
    }

    inline QListWidget *availableItemsWidget() {
      return m_ui->m_listAvailableActions;
    }

  private slots:
    // Insert common controls.
    void insertSpacer();
    void insertSeparator();

  private:
    Ui::ToolBarEditor *m_ui;
    BaseToolBar *m_toolBar;
};

#endif // TOOLBAREDITOR_H
