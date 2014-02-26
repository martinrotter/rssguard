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

#ifndef TEXTFACTORY_H
#define TEXTFACTORY_H

#include "core/defs.h"

#include <QDateTime>
#include <QMap>


class TextFactory {
  private:
    // Constructors and destructors.
    explicit TextFactory();

  public:
    static inline bool isCaseInsensitiveLessThan(const QString &lhs, const QString &rhs) {
      return lhs.toLower() < rhs.toLower();
    }

    // Tries to parse input textual date/time representation.
    // Returns invalid date/time if processing fails.
    // NOTE: This method tries to always return time in UTC+00:00.
    static QDateTime parseDateTime(const QString &date_time);

    // Converts 1970-epoch miliseconds to date/time.
    // NOTE: This method returns date/time local-time
    // which is calculated from the system settings.
    // NOTE: On Windows UTC is known to be broken.
    static QDateTime parseDateTime(qint64 milis_from_epoch);

    // Strips "<....>" (HTML, XML) tags from given text.
    static QString stripTags(QString text);

    // HTML entity escaping.
    // TODO: Optimize these methods.
    static QString escapeHtml(const QString &html);
    static QString deEscapeHtrml(const QString &text);

    static QMap<QString, QString> generetaEscapes();
    static QMap<QString, QString> generateDeescapes();

    // Shortens input string according to given length limit.
    static QString shorten(const QString &input, int text_length_limit = TEXT_TITLE_LIMIT);
};

#endif // TEXTFACTORY_H
