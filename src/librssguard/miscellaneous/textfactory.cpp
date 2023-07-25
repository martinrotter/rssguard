// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textfactory.h"

#include "3rd-party/sc/simplecrypt.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"

#include <QDir>
#include <QLocale>
#include <QRandomGenerator64>
#include <QString>
#include <QStringList>
#include <QTextDocument>

quint64 TextFactory::s_encryptionKey = 0x0;

TextFactory::TextFactory() = default;

QString TextFactory::extractUsernameFromEmail(const QString& email_address) {
  const int zav = email_address.indexOf('@');

  if (zav >= 0) {
    return email_address.mid(0, zav);
  }
  else {
    return email_address;
  }
}

QColor TextFactory::generateColorFromText(const QString& text) {
  quint32 color = 0;

  for (const QChar chr : text) {
    color += chr.unicode();
  }

  // NOTE: https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically
  int hue = color % 360;

  return QColor::fromHsv(hue, 200, 240);
}

int TextFactory::stringHeight(const QString& string, const QFontMetrics& metrics) {
  const int count_lines = string.split(QL1C('\n')).size();

  return metrics.height() * count_lines;
}

int TextFactory::stringWidth(const QString& string, const QFontMetrics& metrics) {
  const QStringList lines = string.split(QL1C('\n'));
  int width = 0;

  for (const QString& line : lines) {
    int line_width = metrics.horizontalAdvance(line);

    if (line_width > width) {
      width = line_width;
    }
  }

  return width;
}

bool TextFactory::couldBeHtml(const QString& string) {
  const QString sstring = string.simplified();

  return sstring.startsWith(QL1S("<!")) || sstring.startsWith(QL1S("<html")) || sstring.startsWith(QL1S("<figure")) ||
         sstring.startsWith(QL1S("<article")) || Qt::mightBeRichText(sstring);
}

QDateTime TextFactory::parseDateTime(const QString& date_time) {
  const QString input_date = date_time.simplified();
  QDateTime dt;
  QTime time_zone_offset;
  const QLocale locale(QLocale::Language::C);
  bool positive_time_zone_offset = false;
  static QStringList date_patterns = dateTimePatterns();
  QStringList timezone_offset_patterns;

  timezone_offset_patterns << QSL("+hh:mm") << QSL("-hh:mm") << QSL("+hhmm") << QSL("-hhmm") << QSL("+hh")
                           << QSL("-hh");

  // Iterate over patterns and check if input date/time matches the pattern.
  for (const QString& pattern : qAsConst(date_patterns)) {
    dt = locale.toDateTime(input_date.left(pattern.size()), pattern);

    if (dt.isValid()) {
      // Make sure that this date/time is considered UTC.
      dt.setTimeSpec(Qt::TimeSpec::UTC);

      // We find offset from UTC.
      if (input_date.size() >= TIMEZONE_OFFSET_LIMIT) {
        QString offset_sanitized = input_date.mid(pattern.size()).replace(QL1S(" "), QString());

        for (const QString& pattern_t : qAsConst(timezone_offset_patterns)) {
          time_zone_offset = QTime::fromString(offset_sanitized.left(pattern_t.size()), pattern_t);

          if (time_zone_offset.isValid()) {
            positive_time_zone_offset = pattern_t.at(0) == QL1C('+');
            break;
          }
        }
      }

      if (time_zone_offset.isValid()) {
        // Time zone offset was detected.
        if (positive_time_zone_offset) {
          // Offset is positive, so we have to subtract it to get
          // the original UTC.
          return dt.addSecs(-QTime(0, 0, 0, 0).secsTo(time_zone_offset));
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
  return QDateTime::fromMSecsSinceEpoch(milis_from_epoch, Qt::TimeSpec::UTC);
}

QStringList TextFactory::dateTimePatterns() {
  return QStringList() << QSL("yyyy-MM-ddTHH:mm:ss") << QSL("MMM dd yyyy hh:mm:ss") << QSL("MMM d yyyy hh:mm:ss")
                       << QSL("ddd, dd MMM yyyy HH:mm:ss") << QSL("ddd, d MMM yyyy HH:mm:ss")
                       << QSL("dd MMM yyyy hh:mm:ss") << QSL("dd MMM yyyy") << QSL("yyyy-MM-dd HH:mm:ss.z")
                       << QSL("yyyy-MM-dd") << QSL("yyyy") << QSL("yyyy-MM") << QSL("yyyy-MM-dd")
                       << QSL("yyyy-MM-ddThh:mm") << QSL("yyyy-MM-ddThh:mm:ss") << QSL("d MMM yyyy HH:mm:ss")
                       << QSL("hh:mm:ss") << QSL("h:m:s AP") << QSL("h:mm") << QSL("H:mm") << QSL("h:m") << QSL("h.m");
}

QString TextFactory::encrypt(const QString& text, quint64 key) {
  return SimpleCrypt(key == 0 ? initializeSecretEncryptionKey() : key).encryptToString(text);
}

QString TextFactory::decrypt(const QString& text, quint64 key) {
  return SimpleCrypt(key == 0 ? initializeSecretEncryptionKey() : key).decryptToString(text);
}

QString TextFactory::newline() {
#if defined(Q_OS_WIN)
  return QSL("\r\n");
#elif defined(Q_OS_MACOS)
  return QSL("\r");
#else
  return QSL("\n");
#endif
}

QString TextFactory::capitalizeFirstLetter(const QString& sts) {
  if (sts.isEmpty()) {
    return sts;
  }
  else {
    return sts[0].toUpper() + sts.mid(1);
  }
}

QStringList TextFactory::tokenizeProcessArguments(QStringView command) {
  QStringList args;
  QString tmp;
  int quote_count = 0;
  bool in_quote = false;

  for (int i = 0; i < command.size(); ++i) {
    if (command.at(i) == QL1C('"')) {
      ++quote_count;

      if (quote_count == 3) {
        quote_count = 0;
        tmp += command.at(i);
      }

      continue;
    }

    if (quote_count) {
      if (quote_count == 1) {
        in_quote = !in_quote;
      }

      quote_count = 0;
    }

    if (!in_quote && command.at(i).isSpace()) {
      if (!tmp.isEmpty()) {
        args += tmp;
        tmp.clear();
      }
    }
    else {
      tmp += command.at(i);
    }
  }

  if (!tmp.isEmpty()) {
    args += tmp;
  }

  return args;
}

QString TextFactory::shorten(const QString& input, int text_length_limit) {
  if (input.size() > text_length_limit) {
    return input.left(text_length_limit - ELLIPSIS_LENGTH) + QString(ELLIPSIS_LENGTH, QL1C('.'));
  }
  else {
    return input;
  }
}

quint64 TextFactory::initializeSecretEncryptionKey() {
  if (s_encryptionKey == 0x0) {
    // Check if file with encryption key exists.
    QString encryption_file_path = qApp->settings()->pathName() + QDir::separator() + ENCRYPTION_FILE_NAME;

    try {
      s_encryptionKey = quint64(QString(IOFactory::readFile(encryption_file_path)).toULongLong());
    }
    catch (ApplicationException&) {
      // Well, key does not exist or is invalid, generate and save one.
      s_encryptionKey = generateSecretEncryptionKey();

      try {
        IOFactory::writeFile(encryption_file_path, QString::number(s_encryptionKey).toLocal8Bit());
      }
      catch (ApplicationException& ex) {
        qCriticalNN << LOGSEC_CORE << "Failed to write newly generated encryption key to file, error"
                    << QUOTE_W_SPACE_DOT(ex.message())
                    << " Now, your passwords won't be readable after you start this application again.";
      }
    }
  }

  return s_encryptionKey;
}

quint64 TextFactory::generateSecretEncryptionKey() {
  return QRandomGenerator64::global()->generate();
}
