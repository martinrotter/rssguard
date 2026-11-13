// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendareventdraft.h"

#include <librssguard/exceptions/applicationexception.h>

#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTest>

namespace {
  CalendarEvent originalEvent() {
    QFile file(QFINDTESTDATA("fixtures/timed-event.json"));
    if (!file.open(QIODevice::ReadOnly)) {
      return {};
    }
    auto resource = QJsonDocument::fromJson(file.readAll()).object();
    resource[QStringLiteral("description")] = QStringLiteral("<p>Hello <b>there</b></p>");
    resource[QStringLiteral("attendees")] =
      QJsonArray{QJsonObject{{QStringLiteral("email"), QStringLiteral("guest@example.invalid")}}};
    resource[QStringLiteral("recurrence")] = QJsonArray{QStringLiteral("RRULE:FREQ=WEEKLY;BYDAY=MO,WE;COUNT=7")};
    return CalendarEvent::fromJson(QStringLiteral("calendar"), resource, "Europe/Prague");
  }
} // namespace

class TestCalendarEventDraft : public QObject {
    Q_OBJECT

  private slots:
    void unchangedAndSparseEditsPreserveResources() {
      const auto original = originalEvent();
      QVERIFY(original.isValid());
      auto draft = CalendarEventDraft::fromEvent(original);
      QVERIFY(draft.changes().isEmpty());
      QCOMPARE(draft.resource(), original.m_resource);
      QCOMPARE(draft.m_fields.m_description, QStringLiteral("Hello there"));
      draft.m_fields.m_title = QStringLiteral("Changed title");
      draft.m_fields.m_location = QStringLiteral("Room 2");
      const auto patch = draft.changes();
      QCOMPARE(patch.size(), 2);
      QVERIFY(!patch.contains(QStringLiteral("start")));
      QVERIFY(!patch.contains(QStringLiteral("description")));
      QVERIFY(!patch.contains(QStringLiteral("recurrence")));
      QCOMPARE(draft.resource().value(QStringLiteral("attendees")),
               original.m_resource.value(QStringLiteral("attendees")));
      QCOMPARE(draft.resource().value(QStringLiteral("conferenceData")),
               original.m_resource.value(QStringLiteral("conferenceData")));
      draft.m_fields.m_description = QStringLiteral("A < B\nSecond line");
      QCOMPARE(draft.changes().value(QStringLiteral("description")).toString(),
               QStringLiteral("A &lt; B<br>Second line"));
    }

    void creationRetainsIdAndConvertsInclusiveAllDayEnd() {
      auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc());
      const auto id = draft.eventId();
      QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-v]{5,1024}$")).match(id).hasMatch());
      draft.m_fields.m_allDay = true;
      draft.m_fields.m_startDate = QDate(2026, 10, 25);
      draft.m_fields.m_endDate = QDate(2026, 10, 27);
      auto resource = draft.changes();
      QCOMPARE(resource.value(QStringLiteral("id")).toString(), id);
      QCOMPARE(resource.value(QStringLiteral("end")).toObject().value(QStringLiteral("date")).toString(),
               QStringLiteral("2026-10-28"));
      draft.m_fields.m_title = QStringLiteral("Retrying the same event");
      QCOMPARE(draft.changes().value(QStringLiteral("id")).toString(), id);
      const auto parsed = CalendarEvent::fromJson(QStringLiteral("calendar"), draft.resource(), "Europe/Prague");
      QVERIFY(parsed.isValid());
      const auto reopened = CalendarEventDraft::fromEvent(parsed);
      QCOMPARE(reopened.m_fields.m_endDate, QDate(2026, 10, 27));
      QVERIFY(reopened.changes().isEmpty());
    }

    void dstRequiresARealUnambiguousChoice() {
      const QTimeZone zone("Europe/Prague");
      QVERIFY(CalendarEventDraft::occurrences(QDate(2026, 3, 29), QTime(2, 30), zone).isEmpty());
      const auto choices = CalendarEventDraft::occurrences(QDate(2026, 10, 25), QTime(2, 30), zone);
      QCOMPARE(choices.size(), 2);
      QCOMPARE(choices.first().secsTo(choices.last()), qint64(3600));
      auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc());
      draft.m_fields.m_startDate = draft.m_fields.m_endDate = QDate(2026, 10, 25);
      draft.m_fields.m_startTime = QTime(2, 30);
      draft.m_fields.m_endTime = QTime(3, 30);
      draft.m_fields.m_startOccurrence = -1;
      QVERIFY_EXCEPTION_THROWN(draft.changes(), ApplicationException);
      draft.m_fields.m_startOccurrence = 1;
      const auto parsed = CalendarEvent::fromJson(QStringLiteral("calendar"), draft.resource(), "Europe/Prague");
      QCOMPARE(parsed.m_start, choices.last());
      draft.m_fields.m_startDate = draft.m_fields.m_endDate = QDate(2026, 3, 29);
      QVERIFY_EXCEPTION_THROWN(draft.changes(), ApplicationException);
    }

    void rebaseRetainsOnlyIntentionalChanges() {
      const auto original = originalEvent();
      auto draft = CalendarEventDraft::fromEvent(original);
      draft.m_fields.m_location = QStringLiteral("Local room");
      auto remote = original.m_resource;
      remote[QStringLiteral("summary")] = QStringLiteral("Remote title");
      remote[QStringLiteral("etag")] = QStringLiteral("version-two");
      draft.rebase(CalendarEvent::fromJson(QStringLiteral("calendar"), remote, "Europe/Prague"));
      QCOMPARE(draft.etag(), QStringLiteral("version-two"));
      QCOMPARE(draft.m_fields.m_title, QStringLiteral("Remote title"));
      QCOMPARE(draft.m_fields.m_location, QStringLiteral("Local room"));
      QCOMPARE(draft.changes().size(), 1);
      QCOMPARE(draft.resource().value(QStringLiteral("recurrence")),
               original.m_resource.value(QStringLiteral("recurrence")));
    }

    void movedOccurrenceKeepsIdentityAndCannotChangeRecurrence() {
      auto resource = originalEvent().m_resource;
      resource.remove(QStringLiteral("recurrence"));
      resource[QStringLiteral("recurringEventId")] = QStringLiteral("master");
      resource[QStringLiteral("originalStartTime")] = resource.value(QStringLiteral("start"));
      const auto original = CalendarEvent::fromJson(QStringLiteral("calendar"), resource, "Europe/Prague");
      auto draft = CalendarEventDraft::fromEvent(original);
      draft.m_fields.m_startDate = draft.m_fields.m_startDate.addDays(1);
      draft.m_fields.m_endDate = draft.m_fields.m_endDate.addDays(1);
      const auto moved = CalendarEvent::fromJson(QStringLiteral("calendar"), draft.resource(), "Europe/Prague");
      QCOMPARE(moved.identity(), original.identity());
      draft.m_fields.m_recurrence = QJsonArray{QStringLiteral("RRULE:FREQ=DAILY")};
      QVERIFY_EXCEPTION_THROWN(draft.changes(), ApplicationException);
    }
};

QTEST_MAIN(TestCalendarEventDraft)
#include "test_calendareventdraft.moc"
