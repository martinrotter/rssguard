// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/httpresponse.h"
#include "network-web/networkfactory.h"

#include <QTest>

class TestHttpResponse : public QObject {
    Q_OBJECT

  private slots:
    void storesBodyAndHeaders();
    void parsesRetryAfter();
};

void TestHttpResponse::storesBodyAndHeaders() {
  HttpResponse response;

  QVERIFY(response.body().isEmpty());
  QVERIFY(response.headers().isEmpty());

  response.setBody(QStringLiteral("response body"));
  response.appendHeader(QStringLiteral("Content-Type"), QStringLiteral("text/plain"));
  response.appendHeader(QStringLiteral("X-Test"), QStringLiteral("value"));

  QCOMPARE(response.body(), QStringLiteral("response body"));
  QCOMPARE(response.headers().size(), 2);
  QCOMPARE(response.headers().at(0), HttpHeader(QStringLiteral("Content-Type"), QStringLiteral("text/plain")));
  QCOMPARE(response.headers().at(1), HttpHeader(QStringLiteral("X-Test"), QStringLiteral("value")));
}

void TestHttpResponse::parsesRetryAfter() {
  const auto retry_at = QDateTime(QDate(2030, 11, 6), QTime(8, 49, 37), Qt::UTC);
  QCOMPARE(NetworkFactory::extractRetryAfter(QStringLiteral("Wed, 06 Nov 2030 08:49:37 GMT")), retry_at);
  QVERIFY(!NetworkFactory::extractRetryAfter(QStringLiteral("invalid date")).isValid());
  const auto before = QDateTime::currentDateTimeUtc();
  const auto seconds = NetworkFactory::extractRetryAfter(QStringLiteral("15"));
  QVERIFY(seconds >= before.addSecs(15) && seconds <= QDateTime::currentDateTimeUtc().addSecs(15));
  for (const auto& header : {QString(), QStringLiteral("0"), QStringLiteral("-1")}) {
    const auto fallback = NetworkFactory::extractRetryAfter(header);
    QVERIFY(fallback >= before.addSecs(120) && fallback <= QDateTime::currentDateTimeUtc().addSecs(120));
  }
}

QTEST_APPLESS_MAIN(TestHttpResponse)

#include "test_httpresponse.moc"
