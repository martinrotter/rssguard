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

#include "miscellaneous/textfactory.h"

#include "definitions/definitions.h"

#include <QString>
#include <QStringList>
#include <QLocale>


TextFactory::TextFactory() {
}

QDateTime TextFactory::parseDateTime(const QString &date_time) {
  QString date = date_time.simplified();
  QStringList date_patterns;
  QStringList timezone_offset_patterns;
  QDateTime dt;
  QTime time_zone_offset;
  QLocale locale(QLocale::C);
  bool positive_time_zone_offset = false;

  date_patterns << "yyyy-MM-ddTHH:mm:ss" << "MMM dd yyyy hh:mm:ss" <<
                   "MMM d yyyy hh:mm:ss" << "ddd, dd MMM yyyy HH:mm:ss" <<
                   "dd MMM yyyy" << "yyyy-MM-dd HH:mm:ss.z" << "yyyy-MM-dd" <<
                   "yyyy" << "yyyy-MM" << "yyyy-MM-dd" << "yyyy-MM-ddThh:mm" <<
                   "yyyy-MM-ddThh:mm:ss";

  timezone_offset_patterns << "+hh:mm" << "-hh:mm" << "+hhmm" << "-hhmm" << "+hh" << "-hh";

  if (date.size() >= TIMEZONE_OFFSET_LIMIT) {
    foreach (const QString &pattern, timezone_offset_patterns) {
      time_zone_offset = QTime::fromString(date.right(pattern.size()), pattern);

      if (time_zone_offset.isValid()) {
        positive_time_zone_offset = pattern.at(0) == '+';
        break;
      }
    }
  }

  // Iterate over patterns and check if input date/time matches the pattern.
  foreach (const QString &pattern, date_patterns) {
    dt = locale.toDateTime(date.left(pattern.size()), pattern);

    if (dt.isValid()) {
      // Make sure that this date/time is considered UTC.
      dt.setTimeSpec(Qt::UTC);

      if (time_zone_offset.isValid()) {
        // Time zone offset was detected.
        if (positive_time_zone_offset) {
          // Offset is positive, so we have to subtract it to get
          // the original UTC.
          return dt.addSecs(- QTime(0, 0, 0, 0).secsTo(time_zone_offset));
        }
        else {
          // Vice versa.
          return dt.addSecs(QTime(0, 0, 0, 0).secsTo(time_zone_offset));
        }
      }
      else {
        return dt;
      }
    }
  }

  // Parsing failed, return invalid datetime.
  return QDateTime();
}

QDateTime TextFactory::parseDateTime(qint64 milis_from_epoch) {
  return QDateTime::fromMSecsSinceEpoch(milis_from_epoch);
}

QString TextFactory::shorten(const QString &input, int text_length_limit) {
  if (input.size() > text_length_limit) {
    return input.left(text_length_limit - ELLIPSIS_LENGTH) + QString(ELLIPSIS_LENGTH, '.');
  }
  else {
    return input;
  }
}
