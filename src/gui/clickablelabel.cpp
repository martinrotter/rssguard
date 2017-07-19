// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "gui/clickablelabel.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QMouseEvent>


ClickableLabel::ClickableLabel(QWidget* parent)
  : QLabel(parent) {
}

QString ClickableLabel::themeIcon() const {
  return m_themeIcon;
}

void ClickableLabel::setThemeIcon(const QString &name) {
  m_themeIcon = name;
  updateIcon();
}

QIcon ClickableLabel::fallbackIcon() const {
  return m_fallbackIcon;
}

void ClickableLabel::setFallbackIcon(const QIcon &fallbackIcon) {
  m_fallbackIcon = fallbackIcon;
  updateIcon();
}

void ClickableLabel::updateIcon() {
  if (!m_themeIcon.isEmpty()) {

    const QIcon icon = qApp->icons()->fromTheme(m_themeIcon);

    if (!icon.isNull()) {
      setPixmap(icon.pixmap(size()));
      return;
    }
  }

  if (!m_fallbackIcon.isNull()) {
    setPixmap(m_fallbackIcon.pixmap(size()));
  }
}

void ClickableLabel::resizeEvent(QResizeEvent *ev) {
  QLabel::resizeEvent(ev);
  updateIcon();
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent* ev) {
  if (ev->button() == Qt::LeftButton && rect().contains(ev->pos())) {
    if (ev->modifiers() == Qt::ControlModifier) {
      emit middleClicked(ev->globalPos());
    }
    else {
      emit clicked(ev->globalPos());
    }
  }
  else if (ev->button() == Qt::MiddleButton && rect().contains(ev->pos())) {
    emit middleClicked(ev->globalPos());
  }
  else {
    QLabel::mouseReleaseEvent(ev);
  }
}
