#ifndef DATE_H
#define DATE_H

#include <QDateTime>


class DateTime {
  private:
    DateTime();

  public:
    // Returns QDatetime instance from input QString.
    // If parsing fails, then invalid QDateTime is returned.
    static QDateTime fromString(const QString &date_time);
};

#endif // DATE_H
