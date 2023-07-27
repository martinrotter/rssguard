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
    static QColor generateColorFromText(const QString& text);
    static QColor generateRandomColor();

    static QString extractUsernameFromEmail(const QString& email_address);
    static int stringHeight(const QString& string, const QFontMetrics& metrics);
    static int stringWidth(const QString& string, const QFontMetrics& metrics);

    static bool couldBeHtml(const QString& string);

    // Tries to parse input textual date/time representation.
    // Returns invalid date/time if processing fails.
    // NOTE: This method tries to always return time in UTC.
    static QDateTime parseDateTime(const QString& date_time);

    // Converts 1970-epoch miliseconds to date/time.
    // NOTE: This method tries to always return time in UTC.
    static QDateTime parseDateTime(qint64 milis_from_epoch);
    static QStringList dateTimePatterns();
    static QString encrypt(const QString& text, quint64 key = 0);
    static QString decrypt(const QString& text, quint64 key = 0);
    static QString newline();
    static QString capitalizeFirstLetter(const QString& sts);
    static QStringList tokenizeProcessArguments(QStringView args);

    // Shortens input string according to given length limit.
    static QString shorten(const QString& input, int text_length_limit = TEXT_TITLE_LIMIT);

  private:
    static quint64 initializeSecretEncryptionKey();
    static quint64 generateSecretEncryptionKey();
    static quint64 s_encryptionKey;
};

#endif // TEXTFACTORY_H
