#ifndef TEXTFACTORY_H
#define TEXTFACTORY_H

#include <QDateTime>

#include "core/defs.h"


class TextFactory {
  private:
    explicit TextFactory();

  public:
    // Tries to parse input textual date/time representation.
    // Returns invalid date/time if processing fails.
    static QDateTime parseDateTime(const QString &date_time);

    // Shortens input string according to given length limit.
    static QString shorten(const QString &input, int text_length_limit = TEXT_TITLE_LIMIT);
};

#endif // TEXTFACTORY_H
