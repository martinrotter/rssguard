#include <QLocale>
#include <QRegExp>
#include <QStringList>

#include "core/datetime.h"


QDateTime DateTime::fromString(const QString &date_time) {
  QString date = date_time.simplified();
  QDateTime dt;
  QString temp;
  QLocale locale(QLocale::C);
  QStringList date_patterns;
  date_patterns << "yyyy-MM-ddTHH:mm:ss" << "MMM dd yyyy hh:mm:ss" <<
                   "MMM hd yyyy hh:mm:ss" << "ddd, dd MMM yyyy HH:mm:ss" <<
                   "dd MMM yyyy" << "yyyy-MM-dd HH:mm:ss.z" << "yyyy-MM-dd";

  // Iterate over patterns and check if input date/time matches the pattern.
  foreach (QString pattern, date_patterns) {
    temp = date.left(pattern.size());
    dt = locale.toDateTime(temp, pattern);
    if (dt.isValid()) {
      return dt;
    }
  }

  qWarning("Problem with parsing date '%s', returning invalid QDateTime instance.",
           qPrintable(date));
  return QDateTime();
}
