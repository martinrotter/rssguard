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

#include "gui/plaintoolbutton.h"

#include <QToolButton>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>


PlainToolButton::PlainToolButton(QWidget *parent) : QToolButton(parent), m_padding(0) {
}

PlainToolButton::~PlainToolButton() {
}

void PlainToolButton::paintEvent(QPaintEvent *e) {
  Q_UNUSED(e)

  QPainter p(this);
  QRect rect(QPoint(0, 0), size());

  // Set padding.
  rect.adjust(m_padding, m_padding, -m_padding, -m_padding);


  // Paint the icon.
  if (underMouse() || isChecked()) {
    p.setOpacity(0.7);
  }

  icon().paint(&p, rect);
}

int PlainToolButton::padding() const {
  return m_padding;
}

void PlainToolButton::setPadding(int padding) {
  m_padding = padding;
  repaint();
}

void PlainToolButton::setChecked(bool checked) {
  QToolButton::setChecked(checked);
  repaint();
}

