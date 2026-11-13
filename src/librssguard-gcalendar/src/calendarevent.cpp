// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendarevent.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QRegularExpression>
#include <QTextDocument>
#include <QUrl>

namespace {
  QDateTime eventDateTime(const QJsonObject& value, const QTimeZone& fallback) {
    const QString text = value.value(QStringLiteral("dateTime")).toString();
    QDateTime result = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!result.isValid()) {
      result = QDateTime::fromString(text, Qt::ISODate);
    }
    static const QRegularExpression offset(QStringLiteral("(?:Z|[+-]\\d{2}:\\d{2})$"));
    if (result.isValid() && !offset.match(text).hasMatch()) {
      const QByteArray zone_id = value.value(QStringLiteral("timeZone")).toString().toUtf8();
      const QTimeZone zone = zone_id.isEmpty() ? fallback : QTimeZone(zone_id);
      if (!zone.isValid()) {
        return {};
      }
      result = QDateTime(result.date(), result.time(), zone);
    }
    return result.toUTC();
  }

  QString plainDescription(const QString& description) {
    QTextDocument document;
    document.setHtml(description);
    return document.toPlainText().toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"));
  }
} // namespace

CalendarEvent CalendarEvent::fromJson(const QString& calendar_id,
                                      const QJsonObject& resource,
                                      const QByteArray& calendar_zone) {
  CalendarEvent result;
  result.m_calendarId = calendar_id;
  result.m_resource = resource;
  const auto start = resource.value(QStringLiteral("start")).toObject();
  const auto end = resource.value(QStringLiteral("end")).toObject();
  const QByteArray zone_id = start.value(QStringLiteral("timeZone")).toString().toUtf8();
  result.m_zone = QTimeZone(zone_id.isEmpty() ? calendar_zone : zone_id);
  const QString remote_id = resource.value(QStringLiteral("id")).toString();
  QJsonArray identity{calendar_id};
  const QString series = resource.value(QStringLiteral("recurringEventId")).toString();
  if (series.isEmpty()) {
    identity.append(QStringLiteral("event"));
    identity.append(remote_id);
  }
  else {
    const auto original = resource.value(QStringLiteral("originalStartTime")).toObject();
    const QString date_text = original.value(QStringLiteral("date")).toString();
    QString original_key;
    if (!date_text.isEmpty()) {
      const QDate date = QDate::fromString(date_text, Qt::ISODate);
      if (date.isValid()) {
        original_key = QStringLiteral("date:") + date.toString(Qt::ISODate);
      }
    }
    else {
      const auto date_time = eventDateTime(original, result.m_zone);
      if (date_time.isValid()) {
        original_key = QStringLiteral("time:") + date_time.toString(Qt::ISODateWithMs);
      }
    }
    if (original_key.isEmpty()) {
      result.m_error = QObject::tr("Invalid recurring event identity.");
      return result;
    }
    identity.append(QStringLiteral("occurrence"));
    identity.append(series);
    identity.append(original_key);
  }
  if (calendar_id.isEmpty() || remote_id.isEmpty()) {
    result.m_error = QObject::tr("Missing calendar or event ID.");
    return result;
  }
  result.m_identity =
    QStringLiteral("gcal:") +
    QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(identity).toJson(QJsonDocument::Compact),
                                                 QCryptographicHash::Sha256)
                          .toHex());
  // Cancelled instances may contain only identity fields; reconciliation uses the cached dates.
  if (result.isCancelled() && (start.isEmpty() || end.isEmpty())) {
    return result;
  }
  const bool all_day = start.contains(QStringLiteral("date"));
  if (all_day) {
    result.m_startDate = QDate::fromString(start.value(QStringLiteral("date")).toString(), Qt::ISODate);
    result.m_endDate = QDate::fromString(end.value(QStringLiteral("date")).toString(), Qt::ISODate);
    if (result.m_zone.isValid()) {
      result.m_start = QDateTime(result.m_startDate, QTime(0, 0), result.m_zone).toUTC();
      result.m_end = QDateTime(result.m_endDate, QTime(0, 0), result.m_zone).toUTC();
    }
  }
  else if (!end.contains(QStringLiteral("date"))) {
    result.m_start = eventDateTime(start, result.m_zone);
    result.m_end = eventDateTime(end, result.m_zone);
  }
  if (!result.m_start.isValid() || !result.m_end.isValid() || result.m_end <= result.m_start ||
      (all_day && (!result.m_endDate.isValid() || result.m_endDate <= result.m_startDate))) {
    result.m_error = QObject::tr("Invalid calendar event time range or time zone.");
  }
  return result;
}

CalendarEvent CalendarEvent::fromMessage(const Message& message, const QByteArray& calendar_zone) {
  const auto document = QJsonDocument::fromJson(message.m_customData.toUtf8());
  const auto envelope = document.object().value(QStringLiteral("gcalendar")).toObject();
  if (!document.isObject() || envelope.value(QStringLiteral("version")).toInt() != 1 ||
      !envelope.value(QStringLiteral("event")).isObject()) {
    CalendarEvent invalid;
    invalid.m_error = QObject::tr("Invalid or unsupported calendar event metadata.");
    return invalid;
  }
  return fromJson(envelope.value(QStringLiteral("calendar_id")).toString(),
                  envelope.value(QStringLiteral("event")).toObject(),
                  envelope.value(QStringLiteral("time_zone")).toString(QString::fromUtf8(calendar_zone)).toUtf8());
}

bool CalendarEvent::isValid() const {
  return m_error.isEmpty() && !m_identity.isEmpty();
}

bool CalendarEvent::isCancelled() const {
  return m_resource.value(QStringLiteral("status")).toString() == QLatin1String("cancelled");
}

bool CalendarEvent::isAllDay() const {
  return m_startDate.isValid();
}

bool CalendarEvent::canEdit(const QString& calendar_access) const {
  const auto type = m_resource.value(QStringLiteral("eventType")).toString();
  const bool ordinary = type.isEmpty() || type == QLatin1String("default");
  const bool writer = calendar_access == QLatin1String("owner") || calendar_access == QLatin1String("writer") ||
                      (calendar_access == QLatin1String("writerWithoutPrivateAccess") &&
                       m_resource.value(QStringLiteral("visibility")).toString() != QLatin1String("private"));
  return isValid() && !isCancelled() && ordinary && writer && !m_resource.value(QStringLiteral("locked")).toBool();
}

QString CalendarEvent::identity() const {
  return m_identity;
}

QString CalendarEvent::displayStart() const {
  return isAllDay() ? QLocale().toString(m_startDate, QLocale::ShortFormat)
                    : QLocale().toString(m_start.toLocalTime(), QLocale::ShortFormat);
}

QString CalendarEvent::displayEnd() const {
  return isAllDay() ? QLocale().toString(m_endDate.addDays(-1), QLocale::ShortFormat)
                    : QLocale().toString(m_end.toLocalTime(), QLocale::ShortFormat);
}

Message CalendarEvent::toMessage() const {
  Message result;
  result.m_customId = identity();
  result.m_feedCustomId = m_calendarId;
  result.m_title = m_resource.value(QStringLiteral("summary")).toString();
  if (result.m_title.isEmpty()) {
    result.m_title = QObject::tr("Untitled event");
  }
  const auto organizer = m_resource.value(QStringLiteral("organizer")).toObject();
  result.m_author = organizer.value(QStringLiteral("displayName")).toString();
  if (result.m_author.isEmpty()) {
    result.m_author = organizer.value(QStringLiteral("email")).toString();
  }
  result.m_url = m_resource.value(QStringLiteral("htmlLink")).toString();
  result.m_created = m_start;
  result.m_createdFromFeed = false;
  result.m_customData =
    QString::fromUtf8(QJsonDocument(QJsonObject{
                                      {QStringLiteral("gcalendar"),
                                       QJsonObject{{QStringLiteral("version"), 1},
                                                   {QStringLiteral("calendar_id"), m_calendarId},
                                                   {QStringLiteral("time_zone"), QString::fromUtf8(m_zone.id())},
                                                   {QStringLiteral("event"), m_resource}}}})
                        .toJson(QJsonDocument::Compact));
  const auto preview_start =
    isAllDay() ? displayStart() : QLocale().toString(m_start.toTimeZone(m_zone), QLocale::ShortFormat);
  const auto preview_end =
    isAllDay() ? displayEnd() : QLocale().toString(m_end.toTimeZone(m_zone), QLocale::ShortFormat);
  result.m_contents = QStringLiteral("<h2>%1</h2><p>%2 &ndash; %3</p><p>%4</p><p>%5</p>")
                        .arg(result.m_title.toHtmlEscaped(),
                             preview_start.toHtmlEscaped(),
                             preview_end.toHtmlEscaped(),
                             m_resource.value(QStringLiteral("location")).toString().toHtmlEscaped(),
                             plainDescription(m_resource.value(QStringLiteral("description")).toString()));
  if (!isAllDay()) {
    result.m_contents +=
      QStringLiteral("<p>%1</p>").arg(QObject::tr("Time zone: %1").arg(QString::fromUtf8(m_zone.id())).toHtmlEscaped());
  }
  if (!result.m_author.isEmpty()) {
    result.m_contents +=
      QStringLiteral("<p>%1</p>").arg(QObject::tr("Organizer: %1").arg(result.m_author).toHtmlEscaped());
  }
  const auto attendees = m_resource.value(QStringLiteral("attendees")).toArray();
  if (!attendees.isEmpty()) {
    result.m_contents += QStringLiteral("<h3>%1</h3><ul>").arg(QObject::tr("Participants").toHtmlEscaped());
    for (const auto& value : attendees) {
      const auto attendee = value.toObject();
      const auto name =
        attendee.value(QStringLiteral("displayName")).toString(attendee.value(QStringLiteral("email")).toString());
      const auto status = attendee.value(QStringLiteral("responseStatus")).toString();
      result.m_contents += QStringLiteral("<li>%1%2</li>")
                             .arg(name.toHtmlEscaped(),
                                  status.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(status.toHtmlEscaped()));
    }
    result.m_contents += QStringLiteral("</ul>");
  }
  for (const auto& value :
       m_resource.value(QStringLiteral("conferenceData")).toObject().value(QStringLiteral("entryPoints")).toArray()) {
    const auto entry = value.toObject();
    const QUrl url(entry.value(QStringLiteral("uri")).toString());
    if (url.isValid() && !url.host().isEmpty() &&
        (url.scheme() == QLatin1String("https") || url.scheme() == QLatin1String("http"))) {
      result.m_contents +=
        QStringLiteral("<p><a href=\"%1\">%2</a></p>")
          .arg(url.toString(QUrl::FullyEncoded).toHtmlEscaped(),
               entry.value(QStringLiteral("label")).toString(QObject::tr("Meeting link")).toHtmlEscaped());
    }
  }
  return result;
}
