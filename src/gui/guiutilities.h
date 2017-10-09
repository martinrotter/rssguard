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

#ifndef GUIUTILITIES_H
#define GUIUTILITIES_H

#include <QIcon>
#include <QLabel>
#include <QWidget>

class GuiUtilities {
  public:
    static void setLabelAsNotice(QLabel& label, bool is_warning);
    static void applyDialogProperties(QWidget& widget, const QIcon& icon = QIcon(), const QString& title = QString());
    static void applyResponsiveDialogResize(QWidget& widget, double factor = 0.6);

  private:
    explicit GuiUtilities();
};

inline GuiUtilities::GuiUtilities() {}

#endif // GUIUTILITIES_H
