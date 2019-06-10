// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/timespinbox.h"

#include <QStringList>

TimeSpinBox::TimeSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {
  setMinimum(3.0);
  setAccelerated(true);
  setDecimals(0);
  setMaximum(10000000.0);
}

TimeSpinBox::~TimeSpinBox() {}

double TimeSpinBox::valueFromText(const QString& text) const {
  bool ok;
  double value = text.toDouble(&ok);

  if (ok) {
    return value;
  }
  else {
    QRegularExpression rx("\\b[0-9]{1,}\\b");
    QStringList numbers;
    int pos = 0;
    int count = 0;
    QRegularExpressionMatchIterator i = rx.globalMatch(text);

    while (i.hasNext()) {
      numbers.append(i.next().captured());

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
  int minutes_total = int(val);
  int minutes_val = minutes_total % 60;
  int hours_val = (minutes_total - minutes_val) / 60;
  QString hours = tr("%n hour(s)", "", hours_val);
  QString minutes = tr("%n minute(s)", "", minutes_val);

  return hours + tr(" and ") + minutes;
}

void TimeSpinBox::fixup(QString& input) const {
  bool ok;
  double value = input.toDouble(&ok);

  if (ok) {
    input = textFromValue(value);
  }
}

QValidator::State TimeSpinBox::validate(QString& input, int& pos) const {
  Q_UNUSED(pos)
  return (valueFromText(input) != -1.0) ? QValidator::Acceptable : QValidator::Intermediate;
}
