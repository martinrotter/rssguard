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

#ifndef TABBAR_H
#define TABBAR_H

#include "miscellaneous/iconfactory.h"

#include <QTabBar>
#include <QVariant>


class TabBar : public QTabBar {
    Q_OBJECT

  public:
    enum TabType {
      FeedReader      = 1,
      DownloadManager = 2,
      NonClosable     = 4,
      Closable        = 8
    };

    // Constructors.
    explicit TabBar(QWidget *parent = 0);
    virtual ~TabBar();

    // Getter/setter for tab type.
    void setTabType(int index, const TabBar::TabType &type);

    inline TabBar::TabType tabType(int index) const {
      return static_cast<TabBar::TabType>(tabData(index).value<int>());
    }

  private slots:
    // Called when user selects to close tab via close button.
    void closeTabViaButton();

  private:
    // Reimplementations.
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

  signals:
    // Emmited if empty space on tab bar is double clicked.
    void emptySpaceDoubleClicked();
};

#endif // TABBAR_H
