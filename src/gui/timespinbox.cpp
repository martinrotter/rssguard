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

#include "gui/timespinbox.h"

#include <QStringList>


TimeSpinBox::TimeSpinBox(QWidget *parent) : QDoubleSpinBox(parent) {
  setMinimum(5.0);
  setAccelerated(true);
  setDecimals(0);
  setMaximum(10000000.0);
}

TimeSpinBox::~TimeSpinBox() {
}

double TimeSpinBox::valueFromText(const QString &text) const {
  bool ok;
  double value = text.toDouble(&ok);

  if (ok) {
    return value;
  }
  else {
    QRegExp rx("\\b[0-9]{1,}\\b");
    QStringList numbers;
    int pos = 0;
    int count = 0;

    while ((pos = rx.indexIn(text, pos)) != -1) {
      numbers.append(rx.cap(0));

      if (pos >= 0) {
        ++pos;
        ++count;
      }
    }

    if (numbers.size() == 2) {
      return (numbers.at(0).toDouble() * 60.0) + numbers.at(1).toDouble();
    }
    else {
      return -1.0;
    }
  }
}

QString TimeSpinBox::textFromValue(double val) const {
  int minutes_total = (int)val;
  int minutes_val = minutes_total % 60;
  int hours_val = (minutes_total - minutes_val) / 60;

  QString hours = tr("%n hour(s)", "", hours_val);
  QString minutes = tr("%n minute(s)", "", minutes_val);

  return hours + tr(" and ") + minutes;
}

void TimeSpinBox::fixup(QString &input) const {
  bool ok;
  double value = input.toDouble(&ok);

  if (ok) {
    input = textFromValue(value);
  }
}

QValidator::State TimeSpinBox::validate(QString &input, int &pos) const {
  Q_UNUSED(pos)

  return (valueFromText(input) != -1.0) ? QValidator::Acceptable : QValidator::Intermediate;
}
