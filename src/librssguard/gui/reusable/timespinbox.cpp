// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/timespinbox.h"

#include "definitions/definitions.h"

#include <QStringList>

TimeSpinBox::TimeSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {
  setAccelerated(true);
  setMinimum(1.0);
  setMaximum(10000000.0);
  setMode(TimeSpinBox::Mode::HoursMinutes);
}

double TimeSpinBox::valueFromText(const QString& text) const {
  bool ok;
  double value = text.toDouble(&ok);

  if (ok) {
    return value;
  }
  else {
    static QRegularExpression rx(QSL("\\b[0-9]{1,}\\b"));
    QStringList numbers;
    QRegularExpressionMatchIterator i = rx.globalMatch(text);

    while (i.hasNext()) {
      numbers.append(i.next().captured());
    }

    if (numbers.size() == 2) {
      switch (m_mode) {
        case TimeSpinBox::Mode::DaysHours:
          return (numbers.at(0).toDouble() * 24.0) + numbers.at(1).toDouble();

        default:
        case TimeSpinBox::Mode::HoursMinutes:
        case TimeSpinBox::Mode::MinutesSeconds:
          return (numbers.at(0).toDouble() * 60.0) + numbers.at(1).toDouble();
      }
    }
    else {
      return -1.0;
    }
  }
}

QString TimeSpinBox::textFromValue(double val) const {
  switch (m_mode) {
    case TimeSpinBox::Mode::MinutesSeconds: {
      // "val" is number of seconds.
      int seconds_val = int(val);
      int minutes_total = seconds_val / 60;
      int seconds_total = seconds_val - (minutes_total * 60);
      QString seconds = tr("%n second(s)", nullptr, seconds_total);
      QString minutes = tr("%n minute(s)", nullptr, minutes_total);

      return minutes + tr(" and ") + seconds;
    }

    case TimeSpinBox::Mode::DaysHours: {
      // "val" is number of hours.
      int hours_val = int(val);
      int days_total = hours_val / 24;
      int hours_total = hours_val - (days_total * 24);
      QString hours = tr("%n hour(s)", nullptr, hours_total);
      QString days = tr("%n day(s)", nullptr, days_total);

      return days + tr(" and ") + hours;
    }

    default:
    case TimeSpinBox::Mode::HoursMinutes: {
      // "val" is number of minutes.
      int minutes_total = int(val);
      int minutes_val = minutes_total % 60;
      int hours_val = (minutes_total - minutes_val) / 60;
      QString hours = tr("%n hour(s)", nullptr, hours_val);
      QString minutes = tr("%n minute(s)", nullptr, minutes_val);

      return hours + tr(" and ") + minutes;
    }
  }
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
  return (valueFromText(input) != -1.0) ? QValidator::State::Acceptable : QValidator::State::Intermediate;
}

TimeSpinBox::Mode TimeSpinBox::mode() const {
  return m_mode;
}

void TimeSpinBox::setMode(TimeSpinBox::Mode mode) {
  m_mode = mode;

  setValue(value());
}
