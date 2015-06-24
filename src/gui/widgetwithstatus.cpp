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

#include "gui/widgetwithstatus.h"

#include "gui/plaintoolbutton.h"
#include "miscellaneous/iconfactory.h"

#include <QHBoxLayout>


WidgetWithStatus::WidgetWithStatus(QWidget *parent)
  : QWidget(parent), m_wdgInput(NULL) {
  m_layout = new QHBoxLayout(this);
  m_btnStatus = new PlainToolButton(this);
  m_btnStatus->setFocusPolicy(Qt::NoFocus);

  m_iconInformation = qApp->icons()->fromTheme(QSL("dialog-information"));
  m_iconWarning = qApp->icons()->fromTheme(QSL("dialog-warning"));
  m_iconError = qApp->icons()->fromTheme(QSL("dialog-error"));
  m_iconOk = qApp->icons()->fromTheme(QSL("dialog-yes"));

  // Set layout properties.
  m_layout->setMargin(0);

  setLayout(m_layout);
  setStatus(Information, QString());
}

WidgetWithStatus::~WidgetWithStatus() {
}

void WidgetWithStatus::setStatus(WidgetWithStatus::StatusType status, const QString &tooltip_text) {
  m_status = status;

  switch (status) {
    case Information:
      m_btnStatus->setIcon(m_iconInformation);
      break;

    case Warning:
      m_btnStatus->setIcon(m_iconWarning);
      break;

    case Error:
      m_btnStatus->setIcon(m_iconError);
      break;

    case Ok:
      m_btnStatus->setIcon(m_iconOk);
      break;

    default:
      break;
  }

  // Setup the tooltip text.
  m_btnStatus->setToolTip(tooltip_text);
}
