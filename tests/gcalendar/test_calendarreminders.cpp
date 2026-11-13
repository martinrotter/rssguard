// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendarreminders.h"

#include <librssguard/exceptions/applicationexception.h>

#include <QJsonDocument>
#include <QTest>

namespace {
  const QDateTime start = QDateTime::fromString(QStringLiteral("2026-11-12T13:00:00Z"), Qt::ISODate);

  Message eventMessage(const QDateTime& date = start) {
    const QJsonObject resource{
      {QStringLiteral("id"), QStringLiteral("event")},
      {QStringLiteral("summary"), QStringLiteral("Discussion")},
      {QStringLiteral("location"), QStringLiteral("Office")},
      {QStringLiteral("start"), QJsonObject{{QStringLiteral("dateTime"), date.toString(Qt::ISODate)}}},
      {QStringLiteral("end"), QJsonObject{{QStringLiteral("dateTime"), date.addSecs(3600).toString(Qt::ISODate)}}}};
    return CalendarEvent::fromJson(QStringLiteral("calendar"), resource).toMessage();
  }

  CalendarReminderSettings enabledSettings() {
    CalendarReminderSettings settings;
    settings.m_enabled = true;
    settings.m_enabledAt = start.addDays(-1);
    return settings;
  }

  QList<CalendarReminder> due(const Message& message,
                              const CalendarReminderSettings& settings,
                              const QJsonObject& ledger = {},
                              const QDateTime& now = start) {
    return CalendarReminders::due(message, settings, ledger, now, QStringLiteral("Work"), "Europe/Prague");
  }
} // namespace

class TestCalendarReminders : public QObject {
    Q_OBJECT

  private slots:
    void googleMaximumOffsetAndCacheWindow() {
      auto settings = enabledSettings();
      settings.m_useGoogleReminders = true;
      QVERIFY_EXCEPTION_THROWN(settings.validateCacheWindow(27), ApplicationException);
      settings.validateCacheWindow(28);
      settings.m_enabledAt = start.addDays(-30);
      const QJsonArray defaults{
        QJsonObject{{QStringLiteral("method"), QStringLiteral("popup")}, {QStringLiteral("minutes"), 40320}}};
      const auto result = CalendarReminders::due(eventMessage(),
                                                 settings,
                                                 {},
                                                 start.addDays(-28),
                                                 QStringLiteral("Work"),
                                                 "UTC",
                                                 defaults);
      QCOMPARE(result.size(), 1);
      QCOMPARE(result.first().m_scheduledFor, start.addDays(-28));
    }

    void defaultAndSettingsRoundTrip() {
      CalendarReminderSettings defaults;
      QCOMPARE(defaults.m_checkIntervalMinutes, 30);
      QVERIFY(!defaults.m_enabled);
      const auto decoded = CalendarReminderSettings::fromJson(enabledSettings().toJson());
      QVERIFY(decoded.m_enabled);
      QCOMPARE(decoded.m_enabledAt, enabledSettings().m_enabledAt);
      QCOMPARE(decoded.m_checkIntervalMinutes, 30);
      auto invalid = decoded.toJson();
      invalid[QStringLiteral("check_interval_minutes")] = 0;
      QVERIFY_EXCEPTION_THROWN(CalendarReminderSettings::fromJson(invalid), ApplicationException);
    }

    void dueAndCatchUpBoundaries() {
      const auto message = eventMessage();
      const auto settings = enabledSettings();
      QVERIFY(due(message, settings, {}, start.addSecs(-1)).isEmpty());
      QCOMPARE(due(message, settings).size(), 1);
      QCOMPARE(due(message, settings, {}, start.addSecs(1800)).size(), 1);
      QVERIFY(due(message, settings, {}, start.addSecs(1801)).isEmpty());
      auto late_enable = settings;
      late_enable.m_enabledAt = start.addSecs(1);
      QVERIFY(due(message, late_enable, {}, start.addSecs(2)).isEmpty());
    }

    void retainedLocalRepeatRoundTripAndLegacyFallback() {
      const auto key = QStringLiteral("local_repeat_minutes");
      auto legacy = enabledSettings().toJson();
      legacy.remove(key);
      auto rules = legacy.value(QStringLiteral("rules")).toArray();
      auto rule = rules.first().toObject();
      rule[QStringLiteral("repeat_minutes")] = 17;
      rules.replace(0, rule);
      legacy[QStringLiteral("rules")] = rules;
      QCOMPARE(CalendarReminderSettings::fromJson(legacy).toJson().value(key).toInt(-1), 17);

      legacy[QStringLiteral("enabled")] = false;
      legacy[QStringLiteral("rules")] = QJsonArray();
      QCOMPARE(CalendarReminderSettings::fromJson(legacy).toJson().value(key).toInt(-1), 0);
      legacy[key] = 23;
      QCOMPARE(CalendarReminderSettings::fromJson(legacy).toJson().value(key).toInt(-1), 23);
      legacy[key] = -1;
      QVERIFY_EXCEPTION_THROWN(CalendarReminderSettings::fromJson(legacy), ApplicationException);
      legacy[key] = 1.5;
      QVERIFY_EXCEPTION_THROWN(CalendarReminderSettings::fromJson(legacy), ApplicationException);
      legacy[key] = 35792;
      QVERIFY_EXCEPTION_THROWN(CalendarReminderSettings::fromJson(legacy), ApplicationException);
    }

    void restartAndMetadataChangesDoNotRepeat() {
      auto message = eventMessage();
      const auto settings = enabledSettings();
      const auto candidates = due(message, settings);
      auto ledger = CalendarReminders::markTriggered({}, candidates, start);
      ledger = QJsonDocument::fromJson(QJsonDocument(ledger).toJson()).object();
      QVERIFY(due(message, settings, ledger, start.addSecs(60)).isEmpty());
      auto raw = CalendarEvent::fromMessage(message).m_resource;
      raw[QStringLiteral("summary")] = QStringLiteral("Renamed");
      message = CalendarEvent::fromJson(QStringLiteral("calendar"), raw).toMessage();
      QVERIFY(due(message, settings, ledger, start.addSecs(60)).isEmpty());
      const auto moved = eventMessage(start.addSecs(600));
      QCOMPARE(due(moved, settings, ledger, start.addSecs(600)).size(), 1);
    }

    void repeatsHaveCutoffAndNoBacklog() {
      auto settings = enabledSettings();
      settings.m_rules.first().m_repeatMinutes = 10;
      const auto message = eventMessage();
      const auto ledger = CalendarReminders::markTriggered({}, due(message, settings), start);
      QVERIFY(due(message, settings, ledger, start.addSecs(599)).isEmpty());
      QCOMPARE(due(message, settings, ledger, start.addSecs(1500)).size(), 1);
      QVERIFY(due(message, settings, ledger, start.addSecs(3600)).isEmpty());
      QVERIFY(due(message, settings, ledger, start.addSecs(-60)).isEmpty());
    }

    void multipleOffsetsAndLocalFlags() {
      auto settings = enabledSettings();
      settings.m_rules.append({QStringLiteral("before"), 15, 0});
      auto message = eventMessage();
      const auto candidates = due(message, settings);
      QCOMPARE(candidates.size(), 2);
      QVERIFY(candidates.first().m_key != candidates.last().m_key);
      message.m_isRead = true;
      QCOMPARE(due(message, settings).size(), 2);
      message.m_isDeleted = true;
      QVERIFY(due(message, settings).isEmpty());
      message.m_isDeleted = false;
      message.m_isPdeleted = true;
      QVERIFY(due(message, settings).isEmpty());
    }

    void allDayUsesCalendarZone() {
      auto raw = CalendarEvent::fromMessage(eventMessage()).m_resource;
      raw[QStringLiteral("start")] = QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-10-25")}};
      raw[QStringLiteral("end")] = QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-10-26")}};
      const auto message = CalendarEvent::fromJson(QStringLiteral("calendar"), raw, "Europe/Prague").toMessage();
      auto settings = enabledSettings();
      const auto now = QDateTime::fromString(QStringLiteral("2026-10-25T08:00:00Z"), Qt::ISODate);
      settings.m_enabledAt = now.addDays(-1);
      const auto candidates = due(message, settings, {}, now);
      QCOMPARE(candidates.size(), 1);
      QCOMPARE(candidates.first().m_scheduledFor, now);
    }

    void googleDefaultsAndInvalidLedger() {
      auto settings = enabledSettings();
      settings.m_useGoogleReminders = true;
      const QJsonArray defaults{
        QJsonObject{{QStringLiteral("method"), QStringLiteral("popup")}, {QStringLiteral("minutes"), 15}},
        QJsonObject{{QStringLiteral("method"), QStringLiteral("email")}, {QStringLiteral("minutes"), 0}}};
      const auto candidates = CalendarReminders::due(eventMessage(),
                                                     settings,
                                                     {},
                                                     start.addSecs(-900),
                                                     QStringLiteral("Work"),
                                                     "UTC",
                                                     defaults);
      QCOMPARE(candidates.size(), 1);
      QCOMPARE(candidates.first().m_scheduledFor, start.addSecs(-900));
      QVERIFY_EXCEPTION_THROWN(CalendarReminders::entries(QJsonObject{{QStringLiteral("version"), 99}}),
                               ApplicationException);
    }
    void templatesDoNotExpandEventText() {
      auto raw = CalendarEvent::fromMessage(eventMessage()).m_resource;
      raw[QStringLiteral("summary")] = QStringLiteral("Keep {calendar} literal");
      auto settings = enabledSettings();
      settings.m_title = QStringLiteral("{title}: {calendar}");
      const auto result = due(CalendarEvent::fromJson(QStringLiteral("calendar"), raw).toMessage(), settings);
      QCOMPARE(result.size(), 1);
      QCOMPARE(result.first().m_title, QStringLiteral("Keep {calendar} literal: Work"));
    }

    void calendarOverridesAndSafeLedgerRetirement() {
      const auto account = enabledSettings();
      const auto disabled =
        CalendarReminderSettings::forCalendar(account,
                                              QJsonObject{
                                                {QStringLiteral("reminder_override"),
                                                 QJsonObject{{QStringLiteral("version"), 1},
                                                             {QStringLiteral("mode"), QStringLiteral("disabled")}}}});
      QVERIFY(!disabled.m_enabled);
      QJsonObject override{{QStringLiteral("version"), 1},
                           {QStringLiteral("mode"), QStringLiteral("local")},
                           {QStringLiteral("enabled_at"), account.m_enabledAt.toString(Qt::ISODateWithMs)},
                           {QStringLiteral("rules"),
                            QJsonArray{QJsonObject{{QStringLiteral("id"), QStringLiteral("custom")},
                                                   {QStringLiteral("minutes_before"), 20}}}}};
      const auto effective =
        CalendarReminderSettings::forCalendar(account, QJsonObject{{QStringLiteral("reminder_override"), override}});
      QCOMPARE(effective.m_checkIntervalMinutes, 30);
      QCOMPARE(effective.m_body, account.m_body);
      QCOMPARE(effective.m_rules.size(), 1);
      QCOMPARE(effective.m_rules.first().m_minutesBefore, 20);
      override[QStringLiteral("mode")] = QStringLiteral("invalid");
      QVERIFY_EXCEPTION_THROWN(CalendarReminderSettings::forCalendar(account,
                                                                     QJsonObject{{QStringLiteral("reminder_override"),
                                                                                  override}}),
                               ApplicationException);
      const auto candidates = due(eventMessage(), account);
      auto ledger = CalendarReminders::markTriggered({}, candidates, start);
      auto entries = CalendarReminders::entries(ledger);
      entries[QStringLiteral("legacy-unknown")] =
        QJsonObject{{QStringLiteral("scheduled_for"), start.toString(Qt::ISODateWithMs)}};
      ledger[QStringLiteral("entries")] = entries;
      const auto retired = CalendarReminders::retire(ledger, {candidates.first().m_eventId});
      QCOMPARE(CalendarReminders::entries(retired).size(), 1);
      QVERIFY(CalendarReminders::entries(retired).contains(QStringLiteral("legacy-unknown")));
    }

    void ruleActivationAndGoogleAllDaySemantics() {
      auto settings = enabledSettings();
      settings.m_rules.first().m_enabledAt = start.addSecs(1);
      QVERIFY(due(eventMessage(), settings, {}, start.addSecs(10)).isEmpty());
      const auto restored = CalendarReminderSettings::fromJson(settings.toJson());
      QCOMPARE(restored.m_rules.first().m_enabledAt, settings.m_rules.first().m_enabledAt);
      auto raw = CalendarEvent::fromMessage(eventMessage()).m_resource;
      raw[QStringLiteral("start")] = QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-10-25")}};
      raw[QStringLiteral("end")] = QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-10-26")}};
      const auto message = CalendarEvent::fromJson(QStringLiteral("calendar"), raw, "Europe/Prague").toMessage();
      settings.m_useGoogleReminders = true;
      settings.m_enabledAt = QDateTime::fromString(QStringLiteral("2026-10-01T00:00:00Z"), Qt::ISODate);
      const auto midnight = QDateTime::fromString(QStringLiteral("2026-10-24T22:00:00Z"), Qt::ISODate);
      const QJsonArray defaults{
        QJsonObject{{QStringLiteral("method"), QStringLiteral("popup")}, {QStringLiteral("minutes"), 0}}};
      const auto candidates =
        CalendarReminders::due(message, settings, {}, midnight, QStringLiteral("Work"), "UTC", defaults);
      QCOMPARE(candidates.size(), 1);
      QCOMPARE(candidates.first().m_scheduledFor, midnight);
      raw[QStringLiteral("reminders")] =
        QJsonObject{{QStringLiteral("useDefault"), false}, {QStringLiteral("overrides"), QJsonArray()}};
      QVERIFY(CalendarReminders::due(CalendarEvent::fromJson(QStringLiteral("calendar"), raw, "Europe/Prague")
                                       .toMessage(),
                                     settings,
                                     {},
                                     midnight,
                                     QStringLiteral("Work"),
                                     "UTC",
                                     defaults)
                .isEmpty());
    }
};

QTEST_MAIN(TestCalendarReminders)
#include "test_calendarreminders.moc"
