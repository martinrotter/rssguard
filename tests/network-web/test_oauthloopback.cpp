// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/httpserver.h"
#include "network-web/oauth2service.h"

#include <QTest>

class TestServer : public HttpServer {
  protected:
    void answerClient(QTcpSocket*, const HttpRequest&) override {}
};

class TestOAuthLoopback : public QObject {
    Q_OBJECT

  private slots:
    void publishesActualEphemeralPort() {
      TestServer first;
      TestServer second;
      first.setListenAddressPort(QStringLiteral("http://127.0.0.1:0/oauth"), true);
      second.setListenAddressPort(QStringLiteral("http://127.0.0.1:0/oauth"), true);
      QVERIFY(first.isListening());
      QVERIFY(second.isListening());
      QVERIFY(first.listenPort() > 0);
      QVERIFY(second.listenPort() > 0);
      QVERIFY(first.listenPort() != second.listenPort());
      const QUrl redirect(first.listenAddressPort());
      QCOMPARE(redirect.port(), int(first.listenPort()));
      QCOMPARE(redirect.path(), QStringLiteral("/oauth"));
      const auto unchanged = first.listenPort();
      first.setListenAddressPort(first.listenAddressPort(), true);
      QCOMPARE(first.listenPort(), unchanged);
    }

    void pkceMatchesRfc7636() {
      QCOMPARE(OAuth2Service::pkceChallenge(QStringLiteral("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk")),
               QStringLiteral("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
    }
};

QTEST_GUILESS_MAIN(TestOAuthLoopback)
#include "test_oauthloopback.moc"
