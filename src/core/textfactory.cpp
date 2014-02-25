#include "core/textfactory.h"

#include "core/defs.h"

#include <QString>
#include <QStringList>
#include <QLocale>
#include <QRegExp>
#include <QMap>


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

QString TextFactory::stripTags(QString text) {
  return text.remove(QRegExp("<[^>]*>"));
}

QString TextFactory::escapeHtml(const QString &html) {
  static QMap<QString, QString> escape_sequences = generetaEscapes();

  QList<QString> keys = escape_sequences.uniqueKeys();
  QString output = html;

  foreach (const QString &key, keys) {
    output.replace(key, escape_sequences.value(key));
  }

  return output;
}

QString TextFactory::deEscapeHtrml(const QString &text) {
  static QMap<QString, QString> deescape_sequences = generateDeescapes();

  QList<QString> keys = deescape_sequences.uniqueKeys();
  QString output = text;

  foreach (const QString &key, keys) {
    output.replace(key, deescape_sequences.value(key));
  }

  return output;
}

QMap<QString, QString> TextFactory::generetaEscapes() {
  QMap<QString, QString> sequences;

  sequences["&lt;"]     = '<';
  sequences["&gt;"]     = '>';
  sequences["&amp;"]		= '&';
  sequences["&quot;"]		= '\"';
  sequences["&nbsp;"]		= ' ';
  sequences["&plusmn;"]	= "±";
  sequences["&times;"]	= "×";
  sequences["&#039;"]   = '\'';

  return sequences;
}

QMap<QString, QString> TextFactory::generateDeescapes() {
  QMap<QString, QString> sequences;

  sequences["<"]	= "&lt;";
  sequences[">"]	= "&gt;";
  sequences["&"]	= "&amp;";
  sequences["\""]	= "&quot;";
  sequences["±"]	= "&plusmn;";
  sequences["×"]	= "&times;";
  sequences["\'"] = "&#039;";

  return sequences;
}
