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
  QDateTime dt;
  QString temp;
  QLocale locale(QLocale::C);
  QStringList date_patterns;
  QStringList timezone_offset_patterns;
  QTime time_zone_offset;
  //QRegExp timezone_rexp;
  bool positive_time_zone_offset = false;

  date_patterns << "yyyy-MM-ddTHH:mm:ss" << "MMM dd yyyy hh:mm:ss" <<
                   "MMM d yyyy hh:mm:ss" << "ddd, dd MMM yyyy HH:mm:ss" <<
                   "dd MMM yyyy" << "yyyy-MM-dd HH:mm:ss.z" << "yyyy-MM-dd" <<
                   "yyyy" << "yyyy-MM" << "yyyy-MM-dd" << "yyyy-MM-ddThh:mm" <<
                   "yyyy-MM-ddThh:mm:ss";

  timezone_offset_patterns << "[+-]\\d\\d:\\d\\d$" <<   // ±[hh]:[mm]
                              "[+-]\\d\\d\\d\\d$" <<    // ±[hh][mm]
                              "[+-]\\d\\d$";            // ±[hh]


  /*if (date.size() > 7) {
    int index = date.size() - 7;

    foreach (const QString &pattern, timezone_offset_patterns) {
      timezone_rexp.setPattern(pattern);

      int index_start_timezone = timezone_rexp.indexIn(date, index);

      if (index_start_timezone != 1) {

      }
    }
  }*/



  // TODO: Patterny revidovat a nejdriv u predaneho stringu
  // hledat +,- znak, je li nalezen tak odriznout vse vpravo od nej
  // a to (mozna pomoci vice patternů) konvertovat na QTime (offset)
  // pote konvertovat zaklad date/time co je vlevo od -,+
  // a na konec pricist/odecist offse.


  // Check if last part of date is time zone offset,
  // represented as [+|-]hh:mm.
  int date_length = date.size();

  if (date_length > 6) {
    char zone_sign = date.at(date_length - 6).toLatin1();

    switch (zone_sign) {
      case '+':
        // Positive time zone offset detected.
        positive_time_zone_offset = true;
        time_zone_offset = QTime::fromString(date.right(5),
                                             "hh:mm");

        date.chop(6);
        break;

      case '-':
        // Negative time zone offset detected.
        time_zone_offset = QTime::fromString(date.right(5),
                                             "hh:mm");

        date.chop(6);
        break;

      default:
        // No time zone offset.
        break;
    }
  }

  // Iterate over patterns and check if input date/time matches the pattern.
  foreach (const QString &pattern, date_patterns) {
    temp = date.left(pattern.size());
    dt = locale.toDateTime(temp, pattern);

    if (dt.isValid()) {
      dt.setTimeSpec(Qt::UTC);

      if (time_zone_offset.isValid()) {
        if (positive_time_zone_offset) {
          return dt.addSecs(- QTime(0, 0, 0, 0).secsTo(time_zone_offset));
        }
        else {
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
