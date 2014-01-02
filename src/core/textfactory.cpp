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
  date_patterns << "yyyy-MM-ddTHH:mm:ss" << "MMM dd yyyy hh:mm:ss" <<
                   "MMM d yyyy hh:mm:ss" << "ddd, dd MMM yyyy HH:mm:ss" <<
                   "dd MMM yyyy" << "yyyy-MM-dd HH:mm:ss.z" << "yyyy-MM-dd" <<
                   "YYYY" << "YYYY-MM" << "YYYY-MM-DD" << "YYYY-MM-DDThh:mmTZD" <<
                   "YYYY-MM-DDThh:mm:ssTZD";

  // Iterate over patterns and check if input date/time matches the pattern.
  foreach (const QString &pattern, date_patterns) {
    temp = date.left(pattern.size());
    dt = locale.toDateTime(temp, pattern);
    if (dt.isValid()) {
      dt.setTimeSpec(Qt::UTC);
      return dt;
    }
  }

  // TODO: ISO time zone offsets are not read now, problem.

  // Parsing failed, return invalid datetime.
  return QDateTime();
}

QDateTime TextFactory::parseDateTime(qint64 milis_from_epoch) {
  QDateTime converted = QDateTime::fromMSecsSinceEpoch(milis_from_epoch);

  // TODO: tadle funkce nahore fromMSec.. by mela vracet cas v UTC
  // tedy timespec Qt::UTC, ale na windows vraci local time.
  // overit co to vraci na linuxu a podle toho
  // prenastavit zobrazovani datumu v messagesmodelu::data (delani
  // nebo nedelani konverze .toLocalTime().
  // mozna taky toString() udela konverzi za me.
  // vsude je pouzito prozatim toLocalTime()
  //converted.setTimeSpec(Qt::UTC);
  return converted;
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
  QMap<QString, QString> sequences;

  sequences["&lt;"]     = '<';
  sequences["&gt;"]     = '>';
  sequences["&amp;"]		= '&';
  sequences["&quot;"]		= '\"';
  sequences["&nbsp;"]		= ' ';
  sequences["&plusmn;"]	= "±";
  sequences["&times;"]	= "×";
  sequences["&#039;"]   = '\'';

  QList<QString> keys = sequences.uniqueKeys();
  QString output = html;

  foreach (const QString &key, keys) {
    output.replace(key, sequences.value(key));
  }

  return output;
}

QString TextFactory::deEscapeHtrml(const QString &text) {
  QMap<QString, QString> sequences;

  sequences["<"]	= "&lt;";
  sequences[">"]	= "&gt;";
  sequences["&"]	= "&amp;";
  sequences["\""]	= "&quot;";
  sequences["±"]	= "&plusmn;";
  sequences["×"]	= "&times;";
  sequences["\'"] = "&#039;";

  QList<QString> keys = sequences.uniqueKeys();
  QString output = text;

  foreach (const QString &key, keys) {
    output.replace(key, sequences.value(key));
  }

  return output;
}
