// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CALENDAREVENTDRAFT_H
#define CALENDAREVENTDRAFT_H

#include "calendarevent.h"

#include <QJsonArray>

class CalendarEventDraft {
  public:
    struct Fields {
        QString m_title;
        QString m_description;
        QString m_location;
        bool m_allDay = false;
        QDate m_startDate;
        QDate m_endDate; // Inclusive for an all-day event.
        QTime m_startTime;
        QTime m_endTime;
        QByteArray m_zone;
        int m_startOccurrence = 0;
        int m_endOccurrence = 0;
        QJsonArray m_recurrence;
    };

    static CalendarEventDraft create(const QByteArray& zone, const QDateTime& now);
    static CalendarEventDraft fromEvent(const CalendarEvent& event);
    static QList<QDateTime> occurrences(const QDate& date, const QTime& time, const QTimeZone& zone);

    QJsonObject changes() const;
    QJsonObject resource() const;
    void rebase(const CalendarEvent& latest);
    bool isNew() const;
    QString eventId() const;
    QString etag() const;
    const QJsonObject& baseline() const;

    Fields m_fields;

  private:
    Fields m_original;
    QJsonObject m_baseline;
    QString m_creationId;
    bool m_new = false;
};

#endif // CALENDAREVENTDRAFT_H
