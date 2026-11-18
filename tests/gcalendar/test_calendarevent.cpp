// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendarevent.h"
#include "gcalendarfeed.h"

#include <librssguard/exceptions/applicationexception.h>

#include <QFile>
#include <QJsonDocument>
#include <QTest>

namespace {
  QJsonObject timedEvent() {
    QFile fixture(QFINDTESTDATA("fixtures/timed-event.json"));
    if (!fixture.open(QIODevice::ReadOnly)) {
      qFatal("Cannot open calendar event fixture.");
    }
    return QJsonDocument::fromJson(fixture.readAll()).object();
  }
} // namespace

class TestCalendarEvent : public QObject {
    Q_OBJECT

  private slots:
    void calendarMetadataPreservesLocalKeys() {
      GCalendarFeed feed;
      const QJsonObject metadata{
        {QStringLiteral("version"), 1},
        {QStringLiteral("reminder_override"), QJsonObject{{QStringLiteral("enabled"), false}}},
        {QStringLiteral("resource"), QJsonObject{{QStringLiteral("id"), QStringLiteral("calendar")}}}};
      feed.setCustomDatabaseData({{QStringLiteral("gcalendar"), metadata.toVariantHash()},
                                  {QStringLiteral("unknown"), 42}});
      feed.setCalendarResource(QJsonObject{{QStringLiteral("id"), QStringLiteral("calendar")},
                                           {QStringLiteral("summary"), QStringLiteral("Work")},
                                           {QStringLiteral("accessRole"), QStringLiteral("reader")}});
      QCOMPARE(feed.customDatabaseData().value(QStringLiteral("unknown")).toInt(), 42);
      QVERIFY(!feed.calendarMetadata()
                 .value(QStringLiteral("reminder_override"))
                 .toObject()
                 .value(QStringLiteral("enabled"))
                 .toBool(true));
      QCOMPARE(feed.calendarResource().value(QStringLiteral("accessRole")).toString(), QStringLiteral("reader"));
      QCOMPARE(feed.title(), QStringLiteral("Work"));
      GCalendarFeed loaded;
      loaded.setCustomDatabaseData(feed.customDatabaseData());
      QCOMPARE(loaded.calendarResource(), feed.calendarResource());
      loaded.setCustomDatabaseData({{QStringLiteral("gcalendar"), QVariantHash{{QStringLiteral("version"), 99}}}});
      const auto before = loaded.customDatabaseData();
      QVERIFY_EXCEPTION_THROWN(loaded.setCalendarResource(feed.calendarResource()), ApplicationException);
      QCOMPARE(loaded.customDatabaseData(), before);
    }

    void messageRoundTripPreservesUnknownFields() {
      auto raw = timedEvent();
      const auto event = CalendarEvent::fromJson(QStringLiteral("calendar@example.com"), raw);
      QVERIFY2(event.isValid(), qPrintable(event.m_error));
      QCOMPARE(event.m_start, QDateTime::fromString(QStringLiteral("2035-11-12T13:00:00Z"), Qt::ISODate));
      const auto message = event.toMessage();
      QVERIFY(!message.m_createdFromFeed);
      QCOMPARE(message.m_created, event.m_start);
      const auto round_trip = CalendarEvent::fromMessage(message);
      QVERIFY(round_trip.isValid());
      QCOMPARE(round_trip.m_resource, raw);
      QCOMPARE(round_trip.identity(), event.identity());
      QVERIFY(CalendarEvent::fromJson(QStringLiteral("another-calendar"), raw).identity() != event.identity());
    }

    void movedOccurrenceKeepsIdentity() {
      auto raw = timedEvent();
      raw[QStringLiteral("recurringEventId")] = QStringLiteral("series");
      raw[QStringLiteral("originalStartTime")] =
        QJsonObject{{QStringLiteral("dateTime"), QStringLiteral("2035-11-12T14:00:00+01:00")}};
      const auto original = CalendarEvent::fromJson(QStringLiteral("calendar"), raw);
      raw[QStringLiteral("start")] = QJsonObject{{QStringLiteral("dateTime"), QStringLiteral("2035-11-13T16:00:00Z")}};
      raw[QStringLiteral("end")] = QJsonObject{{QStringLiteral("dateTime"), QStringLiteral("2035-11-13T17:00:00Z")}};
      raw[QStringLiteral("originalStartTime")] =
        QJsonObject{{QStringLiteral("dateTime"), QStringLiteral("2035-11-12T13:00:00Z")}};
      const auto moved = CalendarEvent::fromJson(QStringLiteral("calendar"), raw);
      QVERIFY(moved.isValid());
      QCOMPARE(moved.identity(), original.identity());
      QVERIFY(moved.m_start != original.m_start);
    }

    void allDayPreservesDatesAcrossDst() {
      auto raw = timedEvent();
      raw[QStringLiteral("start")] = QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-10-25")}};
      raw[QStringLiteral("end")] = QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-10-26")}};
      const auto event = CalendarEvent::fromJson(QStringLiteral("calendar"), raw, "Europe/Prague");
      QVERIFY2(event.isValid(), qPrintable(event.m_error));
      QVERIFY(event.isAllDay());
      QCOMPARE(event.m_startDate, QDate(2026, 10, 25));
      QCOMPARE(event.m_endDate, QDate(2026, 10, 26));
      QCOMPARE(event.m_start.secsTo(event.m_end), qint64(25 * 3600));
      const auto round_trip = CalendarEvent::fromMessage(event.toMessage(), "Europe/Prague");
      QCOMPARE(round_trip.m_startDate, event.m_startDate);
    }

    void cancelledExceptionNeedsOnlyIdentity() {
      const QJsonObject raw{
        {QStringLiteral("id"), QStringLiteral("instance")},
        {QStringLiteral("status"), QStringLiteral("cancelled")},
        {QStringLiteral("recurringEventId"), QStringLiteral("series")},
        {QStringLiteral("originalStartTime"), QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-11-12")}}}};
      const auto event = CalendarEvent::fromJson(QStringLiteral("calendar"), raw);
      QVERIFY(event.isValid());
      QVERIFY(event.isCancelled());
      QVERIFY(!event.canEdit(QStringLiteral("owner")));
      auto complete = timedEvent();
      complete[QStringLiteral("status")] = QStringLiteral("cancelled");
      const auto cached_cancelled = CalendarEvent::fromJson(QStringLiteral("calendar"), complete);
      QVERIFY(cached_cancelled.isValid());
      QVERIFY(cached_cancelled.m_start.isValid());
    }

    void rejectsMalformedData() {
      auto raw = timedEvent();
      raw.remove(QStringLiteral("id"));
      QVERIFY(!CalendarEvent::fromJson(QStringLiteral("calendar"), raw).isValid());
      raw = timedEvent();
      raw[QStringLiteral("end")] = raw.value(QStringLiteral("start"));
      QVERIFY(!CalendarEvent::fromJson(QStringLiteral("calendar"), raw).isValid());
      raw = timedEvent();
      raw[QStringLiteral("recurringEventId")] = QStringLiteral("series");
      QVERIFY(!CalendarEvent::fromJson(QStringLiteral("calendar"), raw).isValid());
      Message message;
      message.m_customData = QStringLiteral("{invalid");
      QVERIFY(!CalendarEvent::fromMessage(message).isValid());
      message.m_customData = QStringLiteral("{\"gcalendar\":{\"version\":99,\"event\":{}}}");
      QVERIFY(!CalendarEvent::fromMessage(message).isValid());
    }

    void permissionsAndEscaping() {
      auto raw = timedEvent();
      raw[QStringLiteral("summary")] = QStringLiteral("<script>alert(1)</script>");
      raw[QStringLiteral("description")] = QStringLiteral("<b>Details</b><script>bad()</script>");
      auto event = CalendarEvent::fromJson(QStringLiteral("calendar"), raw);
      QVERIFY(event.canEdit(QStringLiteral("owner")));
      QVERIFY(event.canEdit(QStringLiteral("writer")));
      QVERIFY(!event.canEdit(QStringLiteral("reader")));
      QVERIFY(!event.toMessage().m_contents.contains(QStringLiteral("<script>")));
      raw[QStringLiteral("locked")] = true;
      QVERIFY(!CalendarEvent::fromJson(QStringLiteral("calendar"), raw).canEdit(QStringLiteral("owner")));
      raw[QStringLiteral("locked")] = false;
      raw[QStringLiteral("eventType")] = QStringLiteral("workingLocation");
      QVERIFY(!CalendarEvent::fromJson(QStringLiteral("calendar"), raw).canEdit(QStringLiteral("owner")));
    }
};

QTEST_MAIN(TestCalendarEvent)
#include "test_calendarevent.moc"
