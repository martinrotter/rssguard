// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendarreminders.h"

#include <librssguard/exceptions/applicationexception.h>
#include <limits>

#include <QCryptographicHash>
#include <QHash>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>

namespace {
  constexpr int maximum_interval = std::numeric_limits<int>::max() / 60000;

  void invalidSettings() {
    throw ApplicationException(QObject::tr("Invalid calendar reminder settings or trigger state."));
  }

  int minutes(const QJsonObject& data, const QString& key, int fallback, int minimum = 0) {
    const auto value = data.value(key);
    if (value.isUndefined()) {
      return fallback;
    }
    const int result = value.toInt(-1);
    if (!value.isDouble() || value.toDouble() != result || result < minimum || result > maximum_interval) {
      invalidSettings();
    }
    return result;
  }

  QString render(const QString& text, const CalendarEvent& event, const QString& calendar_title) {
    const auto summary = event.m_resource.value(QStringLiteral("summary")).toString();
    const QHash<QString, QString> values{
      {QStringLiteral("title"), summary.isEmpty() ? QObject::tr("Untitled event") : summary},
      {QStringLiteral("start"), event.displayStart()},
      {QStringLiteral("end"), event.displayEnd()},
      {QStringLiteral("calendar"), calendar_title},
      {QStringLiteral("location"), event.m_resource.value(QStringLiteral("location")).toString()},
      {QStringLiteral("url"), event.m_resource.value(QStringLiteral("htmlLink")).toString()}};
    static const QRegularExpression placeholder(QStringLiteral("\\{(title|start|end|calendar|location|url)\\}"));
    auto matches = placeholder.globalMatch(text);
    QString result;
    int position = 0;
    while (matches.hasNext()) {
      const auto match = matches.next();
      result += text.mid(position, match.capturedStart() - position);
      result += values.value(match.captured(1));
      position = match.capturedEnd();
    }
    return result + text.mid(position);
  }
} // namespace

QJsonObject CalendarReminderSettings::toJson() const {
  QJsonArray rules;
  for (const auto& rule : m_rules) {
    rules.append(QJsonObject{{QStringLiteral("id"), rule.m_id},
                             {QStringLiteral("minutes_before"), rule.m_minutesBefore},
                             {QStringLiteral("repeat_minutes"), rule.m_repeatMinutes}});
    if (rule.m_enabledAt.isValid()) {
      auto value = rules.last().toObject();
      value[QStringLiteral("enabled_at")] = rule.m_enabledAt.toUTC().toString(Qt::ISODateWithMs);
      rules.replace(rules.size() - 1, value);
    }
  }
  return {{QStringLiteral("version"), 1},
          {QStringLiteral("enabled"), m_enabled},
          {QStringLiteral("use_google_reminders"), m_useGoogleReminders},
          {QStringLiteral("check_interval_minutes"), m_checkIntervalMinutes},
          {QStringLiteral("local_repeat_minutes"), m_localRepeatMinutes},
          {QStringLiteral("enabled_at"), m_enabledAt.toUTC().toString(Qt::ISODateWithMs)},
          {QStringLiteral("all_day_time"), m_allDayTime.toString(QStringLiteral("HH:mm"))},
          {QStringLiteral("title"), m_title},
          {QStringLiteral("body"), m_body},
          {QStringLiteral("rules"), rules}};
}

void CalendarReminderSettings::validateCacheWindow(int future_days) const {
  // Google allows event-specific popup offsets up to four weeks, including events not cached yet.
  int required = m_useGoogleReminders ? 40320 : 0;
  if (!m_useGoogleReminders) {
    for (const auto& rule : m_rules) {
      required = qMax(required, rule.m_minutesBefore);
    }
  }
  if (qint64(required) > qint64(future_days) * 24 * 60) {
    throw ApplicationException(QObject::tr("The future cache window must cover all reminder offsets "
                                           "(at least 28 days for Google's popup reminders)."));
  }
}

CalendarReminderSettings CalendarReminderSettings::fromJson(const QJsonObject& data) {
  CalendarReminderSettings result;
  if (data.isEmpty()) {
    return result;
  }
  if (data.value(QStringLiteral("version")).toInt() != 1) {
    invalidSettings();
  }
  result.m_enabled = data.value(QStringLiteral("enabled")).toBool();
  result.m_useGoogleReminders = data.value(QStringLiteral("use_google_reminders")).toBool();
  result.m_checkIntervalMinutes = minutes(data, QStringLiteral("check_interval_minutes"), 30, 1);
  result.m_enabledAt = QDateTime::fromString(data.value(QStringLiteral("enabled_at")).toString(), Qt::ISODateWithMs);
  result.m_allDayTime = QTime::fromString(data.value(QStringLiteral("all_day_time")).toString(QStringLiteral("09:00")),
                                          QStringLiteral("HH:mm"));
  result.m_title = data.value(QStringLiteral("title")).toString(result.m_title);
  result.m_body = data.value(QStringLiteral("body")).toString(result.m_body);
  if ((result.m_enabled && !result.m_enabledAt.isValid()) || !result.m_allDayTime.isValid()) {
    invalidSettings();
  }
  if (data.contains(QStringLiteral("rules"))) {
    if (!data.value(QStringLiteral("rules")).isArray()) {
      invalidSettings();
    }
    result.m_rules.clear();
    QSet<QString> ids;
    for (const auto& value : data.value(QStringLiteral("rules")).toArray()) {
      const auto rule = value.toObject();
      const QString id = rule.value(QStringLiteral("id")).toString();
      if (!value.isObject() || id.isEmpty() || ids.contains(id)) {
        invalidSettings();
      }
      ids.insert(id);
      result.m_rules.append({id,
                             minutes(rule, QStringLiteral("minutes_before"), 0),
                             minutes(rule, QStringLiteral("repeat_minutes"), 0)});
      if (rule.contains(QStringLiteral("enabled_at"))) {
        result.m_rules.last().m_enabledAt =
          QDateTime::fromString(rule.value(QStringLiteral("enabled_at")).toString(), Qt::ISODateWithMs);
        if (!result.m_rules.last().m_enabledAt.isValid()) {
          invalidSettings();
        }
      }
    }
  }
  result.m_localRepeatMinutes = minutes(data,
                                        QStringLiteral("local_repeat_minutes"),
                                        result.m_rules.isEmpty() ? 0 : result.m_rules.first().m_repeatMinutes);
  return result;
}

CalendarReminderSettings CalendarReminderSettings::forCalendar(const CalendarReminderSettings& account,
                                                               const QJsonObject& metadata) {
  const auto value = metadata.value(QStringLiteral("reminder_override"));
  if (value.isUndefined()) {
    return account;
  }
  const auto override = value.toObject();
  const auto mode = override.value(QStringLiteral("mode")).toString();
  if (!value.isObject() || override.value(QStringLiteral("version")).toInt() != 1 ||
      (mode != QLatin1String("inherit") && mode != QLatin1String("disabled") && mode != QLatin1String("local") &&
       mode != QLatin1String("google"))) {
    invalidSettings();
  }
  auto result = account;
  if (mode == QLatin1String("disabled")) {
    result.m_enabled = false;
    return result;
  }
  const auto enabled =
    QDateTime::fromString(override.value(QStringLiteral("enabled_at")).toString(), Qt::ISODateWithMs);
  if (!enabled.isValid()) {
    invalidSettings();
  }
  if (enabled > result.m_enabledAt) {
    result.m_enabledAt = enabled;
  }
  if (mode != QLatin1String("inherit")) {
    result.m_useGoogleReminders = mode == QLatin1String("google");
    if (mode == QLatin1String("local")) {
      auto settings = result.toJson();
      if (!override.value(QStringLiteral("rules")).isArray()) {
        invalidSettings();
      }
      settings[QStringLiteral("rules")] = override.value(QStringLiteral("rules"));
      result = fromJson(settings);
    }
  }
  return result;
}

QJsonObject CalendarReminders::retire(const QJsonObject& ledger, const QSet<QString>& identities) {
  auto retained = entries(ledger);
  for (auto it = retained.begin(); it != retained.end();) {
    if (identities.contains(it.value().toObject().value(QStringLiteral("event_id")).toString())) {
      it = retained.erase(it);
    }
    else {
      ++it;
    }
  }
  return {{QStringLiteral("version"), 1}, {QStringLiteral("entries"), retained}};
}

QJsonObject CalendarReminders::entries(const QJsonObject& ledger) {
  if (ledger.isEmpty()) {
    return {};
  }
  if (ledger.value(QStringLiteral("version")).toInt() != 1 || !ledger.value(QStringLiteral("entries")).isObject()) {
    invalidSettings();
  }
  return ledger.value(QStringLiteral("entries")).toObject();
}

QList<CalendarReminder> CalendarReminders::due(const Message& message,
                                               const CalendarReminderSettings& settings,
                                               const QJsonObject& ledger,
                                               const QDateTime& now,
                                               const QString& calendar_title,
                                               const QByteArray& calendar_zone,
                                               const QJsonArray& default_reminders) {
  const auto triggered = entries(ledger);
  if (!settings.m_enabled || !settings.m_enabledAt.isValid() || !now.isValid() || message.m_isDeleted ||
      message.m_isPdeleted) {
    return {};
  }
  const auto event = CalendarEvent::fromMessage(message, calendar_zone);
  if (!event.isValid() || event.isCancelled()) {
    return {};
  }
  auto rules = settings.m_rules;
  if (settings.m_useGoogleReminders) {
    rules.clear();
    const auto remote = event.m_resource.value(QStringLiteral("reminders")).toObject();
    const auto offsets = remote.value(QStringLiteral("useDefault")).toBool(true)
                           ? default_reminders
                           : remote.value(QStringLiteral("overrides")).toArray();
    QSet<int> seen;
    for (const auto& offset : offsets) {
      const auto value = offset.toObject();
      const int before = value.value(QStringLiteral("minutes")).toInt(-1);
      if (value.value(QStringLiteral("method")).toString() == QLatin1String("popup") && before >= 0 &&
          before <= 40320 && !seen.contains(before)) {
        seen.insert(before);
        rules.append({QStringLiteral("google-popup-%1").arg(before), before, 0});
      }
    }
  }
  const auto start = event.isAllDay() && !settings.m_useGoogleReminders
                       ? QDateTime(event.m_startDate, settings.m_allDayTime, event.m_zone).toUTC()
                       : event.m_start;
  QList<CalendarReminder> result;
  for (const auto& rule : rules) {
    const auto scheduled = start.addSecs(-qint64(rule.m_minutesBefore) * 60);
    if (!scheduled.isValid() || scheduled < settings.m_enabledAt ||
        (rule.m_enabledAt.isValid() && scheduled < rule.m_enabledAt) || now < scheduled) {
      continue;
    }
    const auto key = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonArray{event.identity(), rule.m_id})
                                                                    .toJson(QJsonDocument::Compact),
                                                                  QCryptographicHash::Sha256)
                                           .toHex());
    const auto state_value = triggered.value(key);
    const auto state = state_value.toObject();
    QDateTime previous_schedule;
    QDateTime last_trigger;
    if (!state_value.isUndefined()) {
      previous_schedule =
        QDateTime::fromString(state.value(QStringLiteral("scheduled_for")).toString(), Qt::ISODateWithMs);
      last_trigger =
        QDateTime::fromString(state.value(QStringLiteral("last_triggered_at")).toString(), Qt::ISODateWithMs);
      if (!state_value.isObject() || !previous_schedule.isValid() || !last_trigger.isValid()) {
        invalidSettings();
      }
    }
    if (previous_schedule == scheduled) {
      if (rule.m_repeatMinutes <= 0 || now >= event.m_end ||
          last_trigger.secsTo(now) < qint64(rule.m_repeatMinutes) * 60) {
        continue;
      }
    }
    else if (scheduled.secsTo(now) > qint64(settings.m_checkIntervalMinutes) * 60) {
      continue;
    }
    result.append({key,
                   event.identity(),
                   message.m_id,
                   scheduled,
                   render(settings.m_title, event, calendar_title),
                   render(settings.m_body, event, calendar_title)});
  }
  return result;
}

QJsonObject CalendarReminders::markTriggered(const QJsonObject& ledger,
                                             const QList<CalendarReminder>& reminders,
                                             const QDateTime& now) {
  auto states = entries(ledger);
  for (const auto& reminder : reminders) {
    states[reminder.m_key] =
      QJsonObject{{QStringLiteral("scheduled_for"), reminder.m_scheduledFor.toUTC().toString(Qt::ISODateWithMs)},
                  {QStringLiteral("event_id"), reminder.m_eventId},
                  {QStringLiteral("last_triggered_at"), now.toUTC().toString(Qt::ISODateWithMs)}};
  }
  return {{QStringLiteral("version"), 1}, {QStringLiteral("entries"), states}};
}
