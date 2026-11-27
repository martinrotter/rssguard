// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CALENDAREVENT_H
#define CALENDAREVENT_H

#include <librssguard/core/message.h>

#include <QJsonObject>
#include <QTimeZone>

class CalendarEvent {
  public:
    static CalendarEvent fromJson(const QString& calendar_id,
                                  const QJsonObject& resource,
                                  const QByteArray& calendar_zone = QByteArray("UTC"));
    static CalendarEvent fromMessage(const Message& message, const QByteArray& calendar_zone = QByteArray("UTC"));

    bool isValid() const;
    bool isCancelled() const;
    bool isAllDay() const;
    bool canEdit(const QString& calendar_access) const;
    QString identity() const;
    QString displayStart() const;
    QString displayEnd() const;
    Message toMessage() const;

    QString m_calendarId;
    QJsonObject m_resource;
    QDateTime m_start;
    QDateTime m_end;
    QDate m_startDate;
    QDate m_endDate;
    QTimeZone m_zone;
    QString m_error;

  private:
    QString m_identity;
};

#endif // CALENDAREVENT_H
