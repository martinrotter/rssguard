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

#include "gui/cornerbutton.h"

#include "gui/iconthemefactory.h"


CornerButton::CornerButton(QWidget *parent) : QToolButton(parent) {
  //: Tooltip for "new tab" webbrowser corner button.
  setToolTip(tr("Open new web browser tab."));
  setAutoRaise(true);
  setIcon(IconThemeFactory::instance()->fromTheme("list-add"));
}

CornerButton::~CornerButton() {
  qDebug("Destroying CornerButton instance.");
}
