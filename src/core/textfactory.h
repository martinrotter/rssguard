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
