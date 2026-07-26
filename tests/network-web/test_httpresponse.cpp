// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/httpresponse.h"

#include <QTest>

class TestHttpResponse : public QObject {
    Q_OBJECT

  private slots:
    void storesBodyAndHeaders();
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

QTEST_APPLESS_MAIN(TestHttpResponse)

#include "test_httpresponse.moc"
