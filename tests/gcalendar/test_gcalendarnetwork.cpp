// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarnetworkfactory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTest>
#include <QtConcurrent>

namespace {
  using Client = GCalendarNetworkFactory;
  using Request = Client::Request;
  using Response = Client::Response;

  Client::Context context() {
    Client::Context result;
    result.m_bearer = QStringLiteral("Bearer test-token");
    result.m_retryDelay = 0;
    return result;
  }

  Response response(const QJsonObject& object, int status = 200) {
    return {status, QJsonDocument(object).toJson(QJsonDocument::Compact), {}, {}};
  }
} // namespace

class TestGCalendarNetwork : public QObject {
    Q_OBJECT

  private slots:
    void authenticationRecovery_data() {
      QTest::addColumn<QByteArray>("method");
      QTest::addColumn<bool>("denied_twice");
      QTest::newRow("read") << QByteArray("GET") << false;
      QTest::newRow("create") << QByteArray("POST") << false;
      QTest::newRow("edit") << QByteArray("PATCH") << false;
      QTest::newRow("bounded-recovery") << QByteArray("GET") << true;
    }

    void authenticationRecovery() {
      QFETCH(QByteArray, method);
      QFETCH(bool, denied_twice);
      auto ctx = context();
      QStringList authorizations;
      ctx.m_authorize = [&](const QString& rejected) {
        authorizations.append(rejected);
        return rejected.isEmpty() ? QStringLiteral("Bearer test-token") : QStringLiteral("Bearer renewed");
      };
      QList<Request> requests;
      Client client(ctx, [&](const Request& request, const Client::Context&) {
        requests.append(request);
        return response(QJsonObject{{QStringLiteral("id"), QStringLiteral("event")}},
                        requests.size() == 1 || denied_twice ? 401 : 200);
      });
      const auto operation = [&]() {
        if (method == "GET") {
          return client.event(QStringLiteral("calendar"), QStringLiteral("event"));
        }
        if (method == "POST") {
          return client.insertEvent(QStringLiteral("calendar"),
                                    {{QStringLiteral("id"), QStringLiteral("event")}},
                                    false);
        }
        return client.updateEvent(QStringLiteral("calendar"),
                                  QStringLiteral("event"),
                                  {{QStringLiteral("summary"), QStringLiteral("Edited")}},
                                  QStringLiteral("etag"),
                                  false);
      };
      if (denied_twice) {
        QVERIFY_EXCEPTION_THROWN(operation(), CalendarApiException);
      }
      else {
        QCOMPARE(operation().value(QStringLiteral("id")).toString(), QStringLiteral("event"));
      }
      QCOMPARE(authorizations, QStringList({QString(), QStringLiteral("Bearer test-token")}));
      QCOMPARE(requests.size(), 2);
      QCOMPARE(requests.first().m_method, method);
      QCOMPARE(requests.last().m_method, method);
      QCOMPARE(requests.first().m_url, requests.last().m_url);
      QCOMPARE(requests.first().m_body, requests.last().m_body);
      QCOMPARE(requests.first().m_headers.value("If-Match"), requests.last().m_headers.value("If-Match"));
      QCOMPARE(requests.first().m_headers.value("Authorization"), QByteArray("Bearer test-token"));
      QCOMPARE(requests.last().m_headers.value("Authorization"), QByteArray("Bearer renewed"));
    }

    void forbiddenDoesNotRefreshAuthorization() {
      auto ctx = context();
      int authorizations = 0;
      ctx.m_authorize = [&](const QString&) {
        ++authorizations;
        return ctx.m_bearer;
      };
      int requests = 0;
      Client client(ctx, [&](const Request&, const Client::Context&) {
        ++requests;
        return response({}, 403);
      });
      QVERIFY_EXCEPTION_THROWN(client.event(QStringLiteral("calendar"), QStringLiteral("event")), CalendarApiException);
      QCOMPARE(authorizations, 1);
      QCOMPARE(requests, 1);
    }

    void failedAuthorizationSendsNoRequest() {
      auto ctx = context();
      ctx.m_authorize = [](const QString&) -> QString {
        throw CalendarApiException(401, QStringLiteral("Refresh failed"));
      };
      int requests = 0;
      Client client(ctx, [&](const Request&, const Client::Context&) {
        ++requests;
        return response({});
      });
      QVERIFY_EXCEPTION_THROWN(client.event(QStringLiteral("calendar"), QStringLiteral("event")), CalendarApiException);
      QCOMPARE(requests, 0);
    }

    void asyncErrorsRetainHttpStatus() {
      Client client(context(), [](const Request&, const Client::Context&) {
        return response(QJsonObject{{QStringLiteral("error"),
                                     QJsonObject{{QStringLiteral("message"), QStringLiteral("Conflict")}}}},
                        412);
      });
      auto future = QtConcurrent::run([client]() {
        return client.event(QStringLiteral("calendar"), QStringLiteral("event"));
      });
      try {
        future.result();
        QFAIL("A failed request must propagate its exception through QFuture.");
      }
      catch (const CalendarApiException& ex) {
        QCOMPARE(ex.m_status, 412);
        QVERIFY(ex.message().contains(QStringLiteral("Conflict")));
      }
    }

    void paginatesWithFixedWindowAndEncodedIds() {
      QList<Request> requests;
      Client client(context(), [&](const Request& request, const Client::Context&) {
        requests.append(request);
        QJsonObject page{
          {QStringLiteral("kind"), QStringLiteral("calendar#events")},
          {QStringLiteral("items"), QJsonArray{QJsonObject{{QStringLiteral("id"), QString::number(requests.size())}}}}};
        if (requests.size() == 1) {
          page[QStringLiteral("nextPageToken")] = QStringLiteral("page+two&");
        }
        return response(page);
      });
      const auto from = QDateTime::fromString(QStringLiteral("2026-11-12T00:00:00Z"), Qt::ISODate);
      const auto until = from.addDays(365);
      const auto events = client.events(QStringLiteral("calendar/a?b@example.com"), from, until);
      QCOMPARE(events.size(), 2);
      QCOMPARE(requests.size(), 2);
      QVERIFY(requests.first()
                .m_url.path(QUrl::FullyEncoded)
                .contains(QStringLiteral("calendar%2Fa%3Fb%40example.com")));
      const QUrlQuery first(requests.first().m_url);
      const QUrlQuery last(requests.last().m_url);
      QCOMPARE(first.queryItemValue(QStringLiteral("timeMin")), last.queryItemValue(QStringLiteral("timeMin")));
      QCOMPARE(first.queryItemValue(QStringLiteral("timeMax")), last.queryItemValue(QStringLiteral("timeMax")));
      QCOMPARE(last.queryItemValue(QStringLiteral("pageToken")), QStringLiteral("page+two&"));
      QVERIFY(!last.hasQueryItem(QStringLiteral("syncToken")));
      QCOMPARE(requests.first().m_headers.value("Authorization"), QByteArray("Bearer test-token"));
    }

    void failedPageAndRepeatedTokenThrow() {
      int calls = 0;
      Client client(context(), [&](const Request&, const Client::Context&) {
        ++calls;
        if (calls == 2) {
          return response(QJsonObject{{QStringLiteral("error"),
                                       QJsonObject{{QStringLiteral("message"), QStringLiteral("Denied")}}}},
                          403);
        }
        return response(QJsonObject{
          {QStringLiteral("kind"), QStringLiteral("calendar#calendarList")},
          {QStringLiteral("items"), QJsonArray{QJsonObject{{QStringLiteral("id"), QStringLiteral("calendar")}}}},
          {QStringLiteral("nextPageToken"), QStringLiteral("next")}});
      });
      QVERIFY_EXCEPTION_THROWN(client.calendars(), CalendarApiException);
      QCOMPARE(calls, 2);
      Client repeated(context(), [](const Request&, const Client::Context&) {
        return response(QJsonObject{{QStringLiteral("kind"), QStringLiteral("calendar#calendarList")},
                                    {QStringLiteral("nextPageToken"), QStringLiteral("same")}});
      });
      QVERIFY_EXCEPTION_THROWN(repeated.calendars(), CalendarApiException);
    }

    void recurringInstancesUseEncodedIdsAndFixedWindow() {
      QList<Request> requests;
      Client client(context(), [&](const Request& request, const Client::Context&) {
        requests.append(request);
        QJsonObject page{
          {QStringLiteral("kind"), QStringLiteral("calendar#events")},
          {QStringLiteral("items"), QJsonArray{QJsonObject{{QStringLiteral("id"), QString::number(requests.size())}}}}};
        if (requests.size() == 1) {
          page[QStringLiteral("nextPageToken")] = QStringLiteral("second");
        }
        return response(page);
      });
      const auto from = QDateTime::fromString(QStringLiteral("2026-11-13T08:00:00Z"), Qt::ISODate);
      QCOMPARE(client.instances(QStringLiteral("calendar/a"), QStringLiteral("series/b"), from, from.addDays(1)).size(),
               2);
      QCOMPARE(requests.size(), 2);
      QVERIFY(requests.first()
                .m_url.path(QUrl::FullyEncoded)
                .endsWith(QStringLiteral("calendar%2Fa/events/series%2Fb/instances")));
      const QUrlQuery first(requests.first().m_url), second(requests.last().m_url);
      QCOMPARE(first.queryItemValue(QStringLiteral("timeMin")), second.queryItemValue(QStringLiteral("timeMin")));
      QCOMPARE(first.queryItemValue(QStringLiteral("timeMax")), second.queryItemValue(QStringLiteral("timeMax")));
      QCOMPARE(second.queryItemValue(QStringLiteral("pageToken")), QStringLiteral("second"));
      QVERIFY(!first.hasQueryItem(QStringLiteral("syncToken")));
    }

    void readsRetryButWritesDoNot() {
      int calls = 0;
      Client client(context(), [&](const Request&, const Client::Context&) {
        return ++calls < 3 ? response({}, 503) : response(QJsonObject{{QStringLiteral("id"), QStringLiteral("event")}});
      });
      QCOMPARE(client.event(QStringLiteral("calendar"), QStringLiteral("event")).value(QStringLiteral("id")).toString(),
               QStringLiteral("event"));
      QCOMPARE(calls, 3);
      calls = 0;
      const QJsonObject resource{{QStringLiteral("id"), Client::newEventId()}};
      QVERIFY_EXCEPTION_THROWN(client.insertEvent(QStringLiteral("calendar"), resource, false), CalendarApiException);
      QCOMPARE(calls, 1);
      QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-v]{5,1024}$"))
                .match(resource.value(QStringLiteral("id")).toString())
                .hasMatch());
    }

    void sparseConditionalUpdatePreservesConflict() {
      QList<Request> requests;
      Client client(context(), [&](const Request& request, const Client::Context&) {
        requests.append(request);
        return response({}, 412);
      });
      try {
        client.updateEvent(QStringLiteral("calendar"),
                           QStringLiteral("event"),
                           QJsonObject{{QStringLiteral("summary"), QStringLiteral("Edited")}},
                           QStringLiteral("etag"),
                           false);
        QFAIL("Expected conflict");
      }
      catch (const CalendarApiException& error) {
        QCOMPARE(error.m_status, 412);
      }
      QCOMPARE(requests.size(), 1);
      QCOMPARE(requests.first().m_method, QByteArray("PATCH"));
      QCOMPARE(requests.first().m_headers.value("If-Match"), QByteArray("etag"));
      QCOMPARE(QJsonDocument::fromJson(requests.first().m_body).object().size(), 1);
      QCOMPARE(QUrlQuery(requests.first().m_url).queryItemValue(QStringLiteral("sendUpdates")), QStringLiteral("none"));
    }

    void duplicateCreateFetchesSameResource() {
      QList<Request> requests;
      const QString id = Client::newEventId();
      Client client(context(), [&](const Request& request, const Client::Context&) {
        requests.append(request);
        return request.m_method == "POST" ? response({}, 409) : response(QJsonObject{{QStringLiteral("id"), id}});
      });
      const auto saved = client.insertEvent(QStringLiteral("calendar"), QJsonObject{{QStringLiteral("id"), id}}, true);
      QCOMPARE(saved.value(QStringLiteral("id")).toString(), id);
      QCOMPARE(requests.size(), 2);
      QVERIFY(requests.last().m_url.path().endsWith(id));
    }

    void cancelledAndMalformedResponses() {
      auto ctx = context();
      ctx.m_cancelled->store(true);
      int calls = 0;
      Client cancelled(ctx, [&](const Request&, const Client::Context&) {
        ++calls;
        return response({});
      });
      QVERIFY_EXCEPTION_THROWN(cancelled.calendars(), CalendarApiException);
      QCOMPARE(calls, 0);
      Client malformed(context(), [](const Request&, const Client::Context&) {
        return Response{200, "not-json", {}, {}};
      });
      QVERIFY_EXCEPTION_THROWN(malformed.calendars(), CalendarApiException);
      Client wrong_collection(context(), [](const Request&, const Client::Context&) {
        return response({});
      });
      QVERIFY_EXCEPTION_THROWN(wrong_collection.calendars(), CalendarApiException);
    }
};

QTEST_GUILESS_MAIN(TestGCalendarNetwork)
#include "test_gcalendarnetwork.moc"
