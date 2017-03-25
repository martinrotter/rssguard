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

#include "gui/squeezelabel.h"


SqueezeLabel::SqueezeLabel(QWidget *parent) : QLabel(parent) {
}

void SqueezeLabel::paintEvent(QPaintEvent *event) {
  if (m_squeezedTextCache != text()) {
    m_squeezedTextCache = text();
    QFontMetrics fm = fontMetrics();

    if (fm.width(m_squeezedTextCache) > contentsRect().width()) {
      setText(fm.elidedText(text(), Qt::ElideMiddle, width()));
    }
  }

  QLabel::paintEvent(event);
}
