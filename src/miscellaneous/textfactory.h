// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "definitions/definitions.h"

#include <QDateTime>
#include <QFontMetrics>


class TextFactory {
  private:
    // Constructors and destructors.
    TextFactory();

  public:
    // Returns true if lhs is smaller than rhs if case-insensitive string comparison is used.
    static inline bool isCaseInsensitiveLessThan(const QString &lhs, const QString &rhs) {
      return lhs.toLower() < rhs.toLower();
    }

    static int stringHeight(const QString &string, const QFontMetrics &metrics);
    static int stringWidth(const QString &string, const QFontMetrics &metrics);

    // Tries to parse input textual date/time representation.
    // Returns invalid date/time if processing fails.
    // NOTE: This method tries to always return time in UTC+00:00.
    static QDateTime parseDateTime(const QString &date_time);

    // Converts 1970-epoch miliseconds to date/time.
    // NOTE: This apparently returns date/time in localtime.
    static QDateTime parseDateTime(qint64 milis_from_epoch);

    static QString encrypt(const QString &text);
    static QString decrypt(const QString &text);

    // Shortens input string according to given length limit.
    static QString shorten(const QString &input, int text_length_limit = TEXT_TITLE_LIMIT);

  private:
    static quint64 initializeSecretEncryptionKey();
    static quint64 generateSecretEncryptionKey();

    static quint64 s_encryptionKey;
};

#endif // TEXTFACTORY_H
