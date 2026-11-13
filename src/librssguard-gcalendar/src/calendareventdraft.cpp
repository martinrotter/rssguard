// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendareventdraft.h"

#include "gcalendarnetworkfactory.h"

#include <algorithm>

#include <QSet>
#include <QTextDocument>

namespace {
  QDateTime selectedTime(const QDate& date, const QTime& time, const QTimeZone& zone, int occurrence) {
    const auto choices = CalendarEventDraft::occurrences(date, time, zone);
    if (choices.isEmpty()) {
      throw ApplicationException(QObject::tr("This local time does not exist in the selected time zone."));
    }
    if (choices.size() > 1 && (occurrence < 0 || occurrence >= choices.size())) {
      throw ApplicationException(QObject::tr("Choose which occurrence of the ambiguous local time to use."));
    }
    return choices.at(choices.size() == 1 ? 0 : occurrence);
  }

  QString plainText(const QString& html) {
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText();
  }
} // namespace

QList<QDateTime> CalendarEventDraft::occurrences(const QDate& date, const QTime& time, const QTimeZone& zone) {
  if (!date.isValid() || !time.isValid() || !zone.isValid()) {
    return {};
  }
  const QDateTime wall(date, time, Qt::UTC);
  QSet<int> offsets;
  for (int day = -2; day <= 2; ++day) {
    offsets.insert(zone.offsetFromUtc(wall.addDays(day)));
  }
  for (const auto& transition : zone.transitions(wall.addDays(-2), wall.addDays(2))) {
    offsets.insert(transition.offsetFromUtc);
  }
  QList<QDateTime> result;
  for (int offset : offsets) {
    const auto candidate = wall.addSecs(-offset);
    const auto local = candidate.toTimeZone(zone);
    if (local.date() == date && local.time() == time) {
      result.append(candidate);
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

CalendarEventDraft CalendarEventDraft::create(const QByteArray& zone, const QDateTime& now) {
  if (!QTimeZone(zone).isValid() || !now.isValid()) {
    throw ApplicationException(QObject::tr("Choose a valid event time zone."));
  }
  const auto start = now.addSecs(3600 - now.time().minute() * 60 - now.time().second());
  const auto end = start.addSecs(3600);
  const QJsonObject resource{{QStringLiteral("id"), GCalendarNetworkFactory::newEventId()},
                             {QStringLiteral("start"),
                              QJsonObject{{QStringLiteral("dateTime"), start.toUTC().toString(Qt::ISODateWithMs)},
                                          {QStringLiteral("timeZone"), QString::fromUtf8(zone)}}},
                             {QStringLiteral("end"),
                              QJsonObject{{QStringLiteral("dateTime"), end.toUTC().toString(Qt::ISODateWithMs)},
                                          {QStringLiteral("timeZone"), QString::fromUtf8(zone)}}}};
  auto result = fromEvent(CalendarEvent::fromJson(QStringLiteral("draft"), resource, zone));
  result.m_creationId = resource.value(QStringLiteral("id")).toString();
  result.m_baseline = {};
  result.m_new = true;
  return result;
}

CalendarEventDraft CalendarEventDraft::fromEvent(const CalendarEvent& event) {
  if (!event.isValid() || event.isCancelled()) {
    throw ApplicationException(event.m_error.isEmpty() ? QObject::tr("This event cannot be edited.") : event.m_error);
  }
  CalendarEventDraft result;
  result.m_baseline = event.m_resource;
  auto& fields = result.m_fields;
  fields.m_title = event.m_resource.value(QStringLiteral("summary")).toString();
  fields.m_description = plainText(event.m_resource.value(QStringLiteral("description")).toString());
  fields.m_location = event.m_resource.value(QStringLiteral("location")).toString();
  fields.m_allDay = event.isAllDay();
  fields.m_zone = event.m_zone.id();
  const auto start = event.m_start.toTimeZone(event.m_zone);
  const auto end = event.m_end.toTimeZone(event.m_zone);
  fields.m_startDate = fields.m_allDay ? event.m_startDate : start.date();
  fields.m_endDate = fields.m_allDay ? event.m_endDate.addDays(-1) : end.date();
  fields.m_startTime = start.time();
  fields.m_endTime = end.time();
  fields.m_startOccurrence = occurrences(start.date(), start.time(), event.m_zone).indexOf(event.m_start);
  fields.m_endOccurrence = occurrences(end.date(), end.time(), event.m_zone).indexOf(event.m_end);
  fields.m_recurrence = event.m_resource.value(QStringLiteral("recurrence")).toArray();
  result.m_original = fields;
  return result;
}

QJsonObject CalendarEventDraft::changes() const {
  QJsonObject result;
  if (m_new || m_fields.m_title != m_original.m_title) {
    result[QStringLiteral("summary")] = m_fields.m_title;
  }
  if (m_new || m_fields.m_description != m_original.m_description) {
    result[QStringLiteral("description")] =
      m_fields.m_description.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"));
  }
  if (m_new || m_fields.m_location != m_original.m_location) {
    result[QStringLiteral("location")] = m_fields.m_location;
  }
  const bool dates_changed = m_fields.m_allDay != m_original.m_allDay || m_fields.m_zone != m_original.m_zone ||
                             m_fields.m_startDate != m_original.m_startDate ||
                             m_fields.m_endDate != m_original.m_endDate ||
                             (!m_fields.m_allDay && (m_fields.m_startTime != m_original.m_startTime ||
                                                     m_fields.m_endTime != m_original.m_endTime ||
                                                     m_fields.m_startOccurrence != m_original.m_startOccurrence ||
                                                     m_fields.m_endOccurrence != m_original.m_endOccurrence));
  if (m_new || dates_changed) {
    const QTimeZone zone(m_fields.m_zone);
    if (!zone.isValid()) {
      throw ApplicationException(QObject::tr("Choose a valid event time zone."));
    }
    QJsonObject start;
    QJsonObject end;
    if (m_fields.m_allDay) {
      if (!m_fields.m_startDate.isValid() || !m_fields.m_endDate.isValid() ||
          m_fields.m_endDate < m_fields.m_startDate || !m_fields.m_endDate.addDays(1).isValid()) {
        throw ApplicationException(QObject::tr("The event's final date must not precede its start date."));
      }
      start[QStringLiteral("date")] = m_fields.m_startDate.toString(Qt::ISODate);
      end[QStringLiteral("date")] = m_fields.m_endDate.addDays(1).toString(Qt::ISODate);
    }
    else {
      const auto start_time =
        selectedTime(m_fields.m_startDate, m_fields.m_startTime, zone, m_fields.m_startOccurrence);
      const auto end_time = selectedTime(m_fields.m_endDate, m_fields.m_endTime, zone, m_fields.m_endOccurrence);
      if (end_time <= start_time) {
        throw ApplicationException(QObject::tr("The event must end after it starts."));
      }
      start = {{QStringLiteral("dateTime"), start_time.toTimeZone(zone).toString(Qt::ISODateWithMs)},
               {QStringLiteral("timeZone"), QString::fromUtf8(m_fields.m_zone)}};
      end = {{QStringLiteral("dateTime"), end_time.toTimeZone(zone).toString(Qt::ISODateWithMs)},
             {QStringLiteral("timeZone"), QString::fromUtf8(m_fields.m_zone)}};
    }
    result[QStringLiteral("start")] = start;
    result[QStringLiteral("end")] = end;
  }
  if (m_fields.m_recurrence != m_original.m_recurrence || (m_new && !m_fields.m_recurrence.isEmpty())) {
    if (!m_new && m_baseline.contains(QStringLiteral("recurringEventId"))) {
      throw ApplicationException(QObject::tr("Edit the whole series to change its recurrence."));
    }
    result[QStringLiteral("recurrence")] = m_fields.m_recurrence;
  }
  if (m_new) {
    result[QStringLiteral("id")] = m_creationId;
  }
  return result;
}

QJsonObject CalendarEventDraft::resource() const {
  auto result = m_baseline;
  const auto patch = changes();
  for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
    result[it.key()] = it.value();
  }
  return result;
}

void CalendarEventDraft::rebase(const CalendarEvent& latest) {
  if (m_new || latest.m_resource.value(QStringLiteral("id")).toString() != eventId()) {
    throw ApplicationException(QObject::tr("The reloaded event does not match this draft."));
  }
  const auto patch = changes();
  auto merged = latest.m_resource;
  for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
    merged[it.key()] = it.value();
  }
  auto replacement = fromEvent(latest);
  replacement.m_fields = fromEvent(CalendarEvent::fromJson(latest.m_calendarId, merged, latest.m_zone.id())).m_fields;
  *this = replacement;
}

bool CalendarEventDraft::isNew() const {
  return m_new;
}
QString CalendarEventDraft::eventId() const {
  return m_new ? m_creationId : m_baseline.value(QStringLiteral("id")).toString();
}
QString CalendarEventDraft::etag() const {
  return m_baseline.value(QStringLiteral("etag")).toString();
}
const QJsonObject& CalendarEventDraft::baseline() const {
  return m_baseline;
}
