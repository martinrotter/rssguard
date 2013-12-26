#ifndef TEXTFACTORY_H
#define TEXTFACTORY_H

#include "core/defs.h"

#include <QDateTime>


class TextFactory {
  private:
    // Constructors and destructors.
    explicit TextFactory();

  public:
    // Tries to parse input textual date/time representation.
    // Returns invalid date/time if processing fails.
    static QDateTime parseDateTime(const QString &date_time);

    // Strips "<....>" (HTML, XML) tags from given text.
    static QString stripTags(QString text);

    // HTML entity escaping.
    // TODO: Optimize these methods.
    static QString escapeHtml(const QString &html);
    static QString deEscapeHtrml(const QString &text);

    // Shortens input string according to given length limit.
    static QString shorten(const QString &input, int text_length_limit = TEXT_TITLE_LIMIT);
};

#endif // TEXTFACTORY_H
