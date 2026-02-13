// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textfactory.h"

#include "3rd-party/sc/simplecrypt.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settings.h"

#if defined(HAS_ICU)
// Direct ICU.
#include <unicode/ucnv.h>
#elif QT_VERSION_MAJOR == 5
#include "qtlinq/qtlinq.h"

// Use QTextCodec on Qt 5.
#include <QTextCodec>
#else
// Use QStringConverter on Qt 6+.
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringEncoder>
#endif

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
  // Compute a stable hash of the input string (MD5 works fine).
  QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Algorithm::Md4);

  // Use first 3 bytes of the hash for RGB components.
  int r = static_cast<unsigned char>(hash[0]);
  int g = static_cast<unsigned char>(hash[1]);
  int b = static_cast<unsigned char>(hash[2]);

  QColor color(r, g, b);

  // Optionally, adjust brightness / saturation for visibility.
#if QT_VERSION_MAJOR == 5
  qreal h, s, l, a;
#else
  float h, s, l, a;
#endif

  color.getHslF(&h, &s, &l, &a);
  s = 0.5 + 0.5 * s; // ensure it's vivid enough
  l = 0.4 + 0.3 * l; // ensure it's not too dark/light
  color.setHslF(h, s, l, a);

  return color;
}

QColor TextFactory::generateRandomColor() {
  int hue = QRandomGenerator::global()->generate() % 360;
  auto clr = QColor::fromHsv(hue, 200, 240);

  return clr;
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

  static QRegularExpression
    tag_exp(QSL("^<("
                "a|abbr|address|area|article|aside|audio|"
                "b|base|bdi|bdo|blockquote|body|br|button|"
                "canvas|caption|cite|code|col|colgroup|"
                "data|datalist|dd|del|details|dfn|dialog|div|dl|dt|"
                "em|embed|"
                "fieldset|figcaption|figure|footer|form|"
                "h1|h2|h3|h4|h5|h6|head|header|hgroup|hr|html|"
                "i|iframe|img|input|ins|"
                "kbd|"
                "label|legend|li|link|"
                "main|map|mark|menu|meta|meter|"
                "nav|noscript|"
                "object|ol|optgroup|option|output|"
                "p|picture|pre|progress|"
                "q|"
                "rp|rt|ruby|"
                "s|samp|script|search|section|select|slot|small|source|span|strong|style|sub|summary|sup|"
                "table|tbody|td|template|textarea|tfoot|th|thead|time|title|tr|track|"
                "u|ul|"
                "var|video|"
                "wbr|"
                "!"
                ")"));

  return tag_exp.match(sstring).hasMatch() || Qt::mightBeRichText(sstring);
}

QString TextFactory::ensureUniqueName(const QString& name, const QStringList& names, const QString& append_format) {
  if (!names.contains(name)) {
    return name;
  }

  QString tmp_name = name;
  int i = 1;

  while (names.contains(tmp_name)) {
    tmp_name = name + append_format.arg(i++);
  }

  return tmp_name;
}

QDateTime TextFactory::parseDateTime(const QString& date_time, QString* used_dt_format) {
  static const QList<QPair<QString, QString>> tz_offsets = {
    {"UTC", "+0000"},  {"GMT", "+0000"},  {"UT", "+0000"},  {"BST", "+0100"}, {"CEST", "+0200"}, {"CET", "+0100"},
    {"EEST", "+0300"}, {"EET", "+0200"},  {"EDT", "-0400"}, {"EST", "-0500"}, {"CDT", "-0500"},  {"CST", "-0600"},
    {"MDT", "-0600"},  {"MST", "-0700"},  {"PDT", "-0700"}, {"PST", "-0800"}, {"AKDT", "-0800"}, {"AKST", "-0900"},
    {"HST", "-1000"},  {"IST", "+0530"},  {"JST", "+0900"}, {"KST", "+0900"}, {"AEST", "+1000"}, {"AEDT", "+1100"},
    {"ACST", "+0930"}, {"ACDT", "+1030"}, {"AWST", "+0800"}};

  QString input_date = date_time.simplified();

  for (const auto& tz : tz_offsets) {
    if (input_date.contains(tz.first)) {
      input_date.replace(tz.first, tz.second);
      break;
    }
  }

  input_date.replace(QRegularExpression(QSL("\\.(\\d{3})\\d+")), QSL(".\\1"));

  if (input_date.isEmpty()) {
    return QDateTime();
  }

  QLocale locale(QLocale::Language::C);
  QDateTime dt;
  QStringList date_patterns = dateTimePatterns(true);

  if (used_dt_format != nullptr && !used_dt_format->isEmpty()) {
    date_patterns.prepend(*used_dt_format);
  }

  for (const QString& pattern : std::as_const(date_patterns)) {
#if QT_VERSION >= 0x060700 // Qt >= 6.7.0
    dt = locale.toDateTime(input_date, pattern, 2000);
#else
    dt = locale.toDateTime(input_date, pattern);
#endif

    if (dt.isValid()) {
      // Make sure that this date/time is considered UTC.
      dt = dt.toUTC();

      if (used_dt_format != nullptr) {
        used_dt_format->clear();
        used_dt_format->append(pattern);
      }

      return dt;
    }
  }

  qCriticalNN << LOGSEC_CORE << "Date/time string NOT recognized:" << QUOTE_W_SPACE_DOT(input_date);
  return QDateTime();
}

QDateTime TextFactory::parseDateTime(qint64 milis_from_epoch) {
  return QDateTime::fromMSecsSinceEpoch(milis_from_epoch,
#if QT_VERSION >= 0x060700 // Qt >= 6.7.0
                                        QTimeZone::utc());
#else
                                        Qt::TimeSpec::UTC);
#endif
}

QStringList TextFactory::dateTimePatterns(bool with_tzs) {
  QStringList pat;

  pat << QSL("yyyy-MM-ddTHH:mm:ss");
  pat << QSL("yyyy-MM-ddTHH:mm:ss.z");
  pat << QSL("yyyy-MM-ddTHH:mm:ss.zzz");
  pat << QSL("yyyy-MM-ddThh:mm:ss");
  pat << QSL("yyyy-MM-dd HH:mm:ss.z");

  pat << QSL("yyyy-MM-ddThh:mm");

  pat << QSL("yyyyMMddThhmmss");
  pat << QSL("yyyyMMdd");
  pat << QSL("yyyy");

  pat << QSL("yyyy-MM-dd");
  pat << QSL("yyyy-MM");

  pat << QSL("MMM dd, yyyy HH:mm:ss");
  pat << QSL("MMM dd yyyy hh:mm:ss");
  pat << QSL("MMM d yyyy hh:mm:ss");

  pat << QSL("ddd, dd MMM yyyy HH:mm:ss");
  pat << QSL("ddd, dd MMM yyyy HH:mm");
  pat << QSL("ddd, dd MMM yy HH:mm:ss");
  pat << QSL("ddd, dd MMMM yyyy HH:mm:ss");
  pat << QSL("ddd, d MMM yyyy HH:mm:ss");

  pat << QSL("ddd, MM/dd/yyyy - HH:mm");

  pat << QSL("dd MMM yyyy hh:mm:ss");
  pat << QSL("dd MMM yyyy hh:mm");
  pat << QSL("dd MMM yyyy");

  pat << QSL("d MMM yyyy HH:mm:ss");
  pat << QSL("d MMM yyyy HH:mm");

  pat << QSL("dd-MM-yyyy - HH:mm");

  pat << QSL("hh:mm:ss");
  pat << QSL("h:m:s");
  pat << QSL("h:mm");
  pat << QSL("H:mm");
  pat << QSL("h:m");
  pat << QSL("h.m");

  if (with_tzs) {
    for (int i = 0; i < pat.size(); i += 3) {
      QString base_pattern = pat.value(i);

      pat.insert(i + 1, base_pattern + QSL("t"));
      pat.insert(i + 2, base_pattern + QSL(" t"));
    }
  }

  return pat;
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

enum class TokenState {
  // We are not inside argument, we are between arguments.
  Normal,

  // We have detected escape "\" character coming from double-quoted argument.
  EscapedFromDoubleQuotes,

  // We have detected escape "\" character coming from spaced argument.
  EscapedFromSpaced,

  // We are inside argument which was separated by spaces.
  InsideArgSpaced,

  // We are inside argument.
  InsideArgDoubleQuotes,

  // We are inside argument, do not evaluate anything, just take it all
  // as arw text.
  InsideArgSingleQuotes
};

QStringList TextFactory::tokenizeProcessArguments(const QString& command) {
  // Each argument containing spaces must be enclosed with single '' or double "" quotes.
  // Some characters must be escaped with \ to keep their textual values as
  // long as double-quoted argument is used.

  if (command.isEmpty()) {
    return {};
  }

  // We append space to end of command to make sure that
  // ending space-separated argument is processed.
  QString my_command = command + u' ';

  TokenState state = TokenState::Normal;
  QStringList args;
  QString arg;

  for (QChar chr : my_command) {
    switch (state) {
      case TokenState::Normal: {
        switch (chr.unicode()) {
          case u'"':
            // We start double-quoted argument.
            state = TokenState::InsideArgDoubleQuotes;
            continue;

          case u'\'':
            // We start single-quoted argument.
            state = TokenState::InsideArgSingleQuotes;
            continue;

          case u' ':
            // Whitespace, just go on.
            continue;

          default:
            // We found some actual text which marks
            // beginning of argument, we assume spaced argument.
            arg.append(chr);
            state = TokenState::InsideArgSpaced;
            continue;
        }

        break;
      }

      case TokenState::EscapedFromDoubleQuotes: {
        // Previous character was "\".
        arg.append(chr);
        state = TokenState::InsideArgDoubleQuotes;
        break;
      }

      case TokenState::EscapedFromSpaced: {
        // Previous character was "\".
        arg.append(chr);
        state = TokenState::InsideArgSpaced;
        break;
      }

      case TokenState::InsideArgSpaced: {
        switch (chr.unicode()) {
            // NOTE: Probably disable escaping in spaced arguments to provide simpler UX?
            /*
            case u'\\':
              // We found escaped!
              state = TokenState::EscapedFromSpaced;
              continue;
            */

          case u' ':
            // We need to end this argument.
            args.append(arg);
            arg.clear();
            state = TokenState::Normal;
            continue;

          default:
            arg.append(chr);
            break;
        }

        break;
      }

      case TokenState::InsideArgDoubleQuotes: {
        switch (chr.unicode()) {
          case u'\\':
            // We found escaped!
            state = TokenState::EscapedFromDoubleQuotes;
            continue;

          case u'"':
            // We need to end this argument.
            args.append(arg);
            arg.clear();
            state = TokenState::Normal;
            continue;

          default:
            arg.append(chr);
            break;
        }

        break;
      }

      case TokenState::InsideArgSingleQuotes: {
        switch (chr.unicode()) {
          case u'\'':
            // We need to end this argument.
            args.append(arg);
            arg.clear();
            state = TokenState::Normal;
            continue;

          default:
            arg.append(chr);
            break;
        }

        break;
      }
    }
  }

  switch (state) {
    case TokenState::EscapedFromSpaced:
    case TokenState::EscapedFromDoubleQuotes:
      throw ApplicationException(QObject::tr("escape sequence not completed"));
      break;

    case TokenState::InsideArgDoubleQuotes:
      throw ApplicationException(QObject::tr("closing \" is missing"));
      break;

    case TokenState::InsideArgSingleQuotes:
      throw ApplicationException(QObject::tr("closing ' is missing"));
      break;

    default:
      break;
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

QString TextFactory::fromEncoding(const QByteArray& data, const QString& encoding) {
  if (encoding.isEmpty()) {
    return QString::fromUtf8(data);
  }

#if defined(HAS_ICU)
  UErrorCode status = U_ZERO_ERROR;
  QByteArray enc = encoding.toUtf8();
  UConverter* conv = ucnv_open(enc.constData(), &status);
  if (U_FAILURE(status)) {
    return {};
  }

  // First pass: size
  int32_t needed = ucnv_toUChars(conv, nullptr, 0, data.constData(), data.size(), &status);
  if (status != U_BUFFER_OVERFLOW_ERROR) {
    ucnv_close(conv);
    return {};
  }

  status = U_ZERO_ERROR;
  QString out(needed, Qt::Uninitialized);
  ucnv_toUChars(conv, reinterpret_cast<UChar*>(out.data()), needed, data.constData(), data.size(), &status);
  ucnv_close(conv);

  return U_SUCCESS(status) ? out : QString{};
#elif QT_VERSION_MAJOR == 5
  auto* codec = QTextCodec::codecForName(encoding.toLocal8Bit());
  return codec->toUnicode(data);
#else
  auto c_encoding_ascii = encoding.toLocal8Bit();
  auto c_encoding = c_encoding_ascii.constData();
  auto decoder = QStringDecoder(c_encoding);
  return decoder.decode(data);
#endif
}

QByteArray TextFactory::toEncoding(const QString& str, const QString& encoding) {
#if defined(HAS_ICU)
  UErrorCode status = U_ZERO_ERROR;
  UConverter* conv = ucnv_open(encoding.toUtf8().constData(), &status);
  if (U_FAILURE(status)) {
    return {};
  }

  const UChar* u = reinterpret_cast<const UChar*>(str.utf16());
  int32_t srcLen = str.size();

  // First pass: determine buffer size
  int32_t bytesNeeded = ucnv_fromUChars(conv, nullptr, 0, u, srcLen, &status);

  if (status != U_BUFFER_OVERFLOW_ERROR) {
    ucnv_close(conv);
    return {};
  }

  // Second pass: actual conversion
  status = U_ZERO_ERROR;
  QByteArray out(bytesNeeded, Qt::Uninitialized);
  ucnv_fromUChars(conv, out.data(), bytesNeeded, u, srcLen, &status);

  ucnv_close(conv);
  return U_SUCCESS(status) ? out : QByteArray();
#elif QT_VERSION_MAJOR == 5
  auto* codec = QTextCodec::codecForName(encoding.toLocal8Bit());
  return codec->fromUnicode(str);
#else
  auto c_encoding_ascii = encoding.toLocal8Bit();
  auto c_encoding = c_encoding_ascii.constData();
  auto encoder = QStringEncoder(c_encoding);
  return encoder.encode(str);
#endif
}

QStringList TextFactory::availableEncodings() {
  static QStringList enc = availableEncodingsInit();

  return enc;
}

QStringList TextFactory::availableEncodingsInit() {
#if defined(HAS_ICU)
  QStringList out;
  QSet<QString> seen;

  UErrorCode status = U_ZERO_ERROR;
  int32_t count = ucnv_countAvailable();

  for (int32_t i = 0; i < count; ++i) {
    const char* canonical = ucnv_getAvailableName(i);

    QString canonStr = QString::fromUtf8(canonical);
    if (!seen.contains(canonStr)) {
      out.append(canonStr);
      seen.insert(canonStr);
    }

    // --- Add all aliases ---
    status = U_ZERO_ERROR;
    int32_t aliasCount = ucnv_countAliases(canonical, &status);

    if (U_SUCCESS(status)) {
      for (int32_t j = 0; j < aliasCount; ++j) {
        auto* c_alias = ucnv_getAlias(canonical, j, &status);
        auto alias = QString::fromLocal8Bit(c_alias);

        if (!seen.contains(alias)) {
          out.append(alias);
          seen.insert(alias);
        }
      }
    }
  }

  return out;
#elif QT_VERSION_MAJOR == 5
  auto codecs = QTextCodec::availableCodecs();

  return qlinq::from(codecs)
    .select([](const QByteArray& codec) {
      return QString::fromLocal8Bit(codec);
    })
    .toList();
#elif QT_VERSION < 0x060700 // Qt < 6.7.0
  QStringList supported_encodings = {QSL("UTF-8"),
                                     QSL("UTF-16"),
                                     QSL("UTF-16BE"),
                                     QSL("UTF-16LE"),
                                     QSL("UTF-32"),
                                     QSL("UTF-32BE"),
                                     QSL("UTF-32LE"),
                                     QSL("ISO-8859-1"),
                                     QSL("System")};

  return supported_encodings;
#else
  return QStringConverter::availableCodecs();
#endif
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
