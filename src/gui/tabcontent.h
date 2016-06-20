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

#ifndef TABCONTENT_H
#define TABCONTENT_H

#include <QWidget>


class WebBrowser;

// Base class for all widgets which are placed inside tabs of TabWidget
class TabContent : public QWidget {
    Q_OBJECT

  public:
    // Contructors.
    explicit TabContent(QWidget *parent = 0);
    virtual ~TabContent();

    // Gets/sets current index of this TabContent.
    // NOTE: This is the index under which this object lies
    // in parent tab widget.
    inline virtual int index() const {
      return m_index;
    }

    inline virtual void setIndex(int index) {
      m_index = index;
    }

    // Obtains instance contained in this TabContent or nullptr.
    // This can be used for obtaining the menu from the instance and so on.
    virtual WebBrowser *webBrowser() const = 0;

  protected:
    int m_index;
};

#endif // TABCONTENT_H
