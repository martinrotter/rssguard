// For license of this file, see <project-root-folder>/LICENSE.md.

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
    static int stringHeight(const QString& string, const QFontMetrics& metrics);
    static int stringWidth(const QString& string, const QFontMetrics& metrics);

    // Tries to parse input textual date/time representation.
    // Returns invalid date/time if processing fails.
    // NOTE: This method tries to always return time in UTC+00:00.
    static QDateTime parseDateTime(const QString& date_time);

    // Converts 1970-epoch miliseconds to date/time.
    // NOTE: This apparently returns date/time in localtime.
    static QDateTime parseDateTime(qint64 milis_from_epoch);
    static QString encrypt(const QString& text);
    static QString decrypt(const QString& text);
    static QString newline();

    // Shortens input string according to given length limit.
    static QString shorten(const QString& input, int text_length_limit = TEXT_TITLE_LIMIT);

  private:
    static quint64 initializeSecretEncryptionKey();
    static quint64 generateSecretEncryptionKey();
    static quint64 s_encryptionKey;
};

#endif // TEXTFACTORY_H
