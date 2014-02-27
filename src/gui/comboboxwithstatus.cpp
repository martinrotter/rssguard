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

#include "gui/comboboxwithstatus.h"

#include "gui/plaintoolbutton.h"

#include <QHBoxLayout>


ComboBoxWithStatus::ComboBoxWithStatus(QWidget *parent)
  : WidgetWithStatus(parent) {
  m_wdgInput = new QComboBox(this);

  // Set correct size for the tool button.
  int txt_input_height = m_wdgInput->sizeHint().height();
  m_btnStatus->setFixedSize(txt_input_height, txt_input_height);

  // Compose the layout.
  m_layout->addWidget(m_wdgInput);
  m_layout->addWidget(m_btnStatus);
}

ComboBoxWithStatus::~ComboBoxWithStatus() {
}
