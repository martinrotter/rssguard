// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CALENDARREMINDERS_H
#define CALENDARREMINDERS_H

#include "calendarevent.h"

#include <QJsonArray>
#include <QSet>

struct CalendarReminderRule {
    QString m_id = QStringLiteral("at-start");
    int m_minutesBefore = 0;
    int m_repeatMinutes = 0;
    QDateTime m_enabledAt;
};

struct CalendarReminderSettings {
    bool m_enabled = false;
    bool m_useGoogleReminders = false;
    int m_checkIntervalMinutes = 30;
    int m_localRepeatMinutes = 0;
    QDateTime m_enabledAt;
    QTime m_allDayTime = QTime(9, 0);
    QString m_title = QStringLiteral("{title}");
    QString m_body = QStringLiteral("{calendar}: {start}\n{location}");
    QList<CalendarReminderRule> m_rules{CalendarReminderRule()};

    QJsonObject toJson() const;
    void validateCacheWindow(int future_days) const;
    static CalendarReminderSettings fromJson(const QJsonObject& data);
    static CalendarReminderSettings forCalendar(const CalendarReminderSettings& account, const QJsonObject& metadata);
};

struct CalendarReminder {
    QString m_key;
    QString m_eventId;
    int m_messageId = 0;
    QDateTime m_scheduledFor;
    QString m_title;
    QString m_body;
};

// Pure plugin-local evaluation. The account owns the timer, database jobs and GUI delivery.
class CalendarReminders {
  public:
    static QList<CalendarReminder> due(const Message& message,
                                       const CalendarReminderSettings& settings,
                                       const QJsonObject& ledger,
                                       const QDateTime& now,
                                       const QString& calendar_title,
                                       const QByteArray& calendar_zone,
                                       const QJsonArray& default_reminders = {});
    static QJsonObject markTriggered(const QJsonObject& ledger,
                                     const QList<CalendarReminder>& reminders,
                                     const QDateTime& now);
    static QJsonObject entries(const QJsonObject& ledger);
    static QJsonObject retire(const QJsonObject& ledger, const QSet<QString>& identities);
};

#endif // CALENDARREMINDERS_H
