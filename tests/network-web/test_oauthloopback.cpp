// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "network-web/httpserver.h"
#include "network-web/oauth2service.h"

#include <QNetworkProxy>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

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
    void tokenExchange_data() {
      QTest::addColumn<bool>("pkce");
      QTest::newRow("legacy") << false;
      QTest::newRow("pkce") << true;
    }

    void tokenExchange();
};

void TestOAuthLoopback::tokenExchange() {
  QFETCH(bool, pkce);
  QTcpServer endpoint;
  QVERIFY(endpoint.listen(QHostAddress::LocalHost));
  QByteArray received;
  QByteArray body = R"({"access_token":"fixture-access","refresh_token":"fixture-refresh","expires_in":3600})";
  connect(&endpoint, &QTcpServer::newConnection, &endpoint, [&]() {
    auto* socket = endpoint.nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
      received += socket->readAll();
      const int header_end = received.indexOf("\r\n\r\n");
      if (header_end < 0) {
        return;
      }
      const auto headers = received.left(header_end).split('\n');
      int content_length = -1;
      for (const auto& header : headers) {
        if (header.toLower().startsWith("content-length:")) {
          content_length = header.mid(15).trimmed().toInt();
        }
      }
      if (content_length < 0 || received.size() < header_end + 4 + content_length) {
        return;
      }
      socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                    QByteArray::number(body.size()) + "\r\n\r\n" + body);
      socket->disconnectFromHost();
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  });
  const auto token_url = QSL("http://127.0.0.1:%1/token").arg(endpoint.serverPort());
  OAuth2Service oauth(QSL("https://example.invalid/authorize"),
                      token_url,
                      QSL("fixture-client"),
                      {},
                      QSL("fixture-scope"));
  if (pkce) {
    oauth.setPkceEnabled(true);
  }
  oauth.setRedirectUrl(QSL("http://127.0.0.1:0"), true);
  const QUrlQuery first(oauth.createAuthorizationUrl());
  const QUrlQuery second(oauth.createAuthorizationUrl());
  QVERIFY(first.queryItemValue(QSL("state")) != second.queryItemValue(QSL("state")));
  QVERIFY(QUrl(second.queryItemValue(QSL("redirect_uri"))).port() > 0);
  QCOMPARE(second.queryItemValue(QSL("code_challenge_method")), pkce ? QSL("S256") : QString());
  QCOMPARE(second.hasQueryItem(QSL("code_challenge")), pkce);
  QSignalSpy tokens(&oauth, &OAuth2Service::tokensRetrieved);
  oauth.retrieveAccessToken(QSL("fixture-code"));
  QTRY_COMPARE_WITH_TIMEOUT(tokens.size(), 1, 5000);
  const QUrlQuery form(QString::fromUtf8(received.mid(received.indexOf("\r\n\r\n") + 4)));
  QCOMPARE(form.queryItemValue(QSL("code")), QSL("fixture-code"));
  QCOMPARE(form.queryItemValue(QSL("redirect_uri")), second.queryItemValue(QSL("redirect_uri")));
  QCOMPARE(form.hasQueryItem(QSL("code_verifier")), pkce);
  if (pkce) {
    QCOMPARE(OAuth2Service::pkceChallenge(form.queryItemValue(QSL("code_verifier"))),
             second.queryItemValue(QSL("code_challenge")));
  }

  QSignalSpy failures(&oauth, &OAuth2Service::tokensRetrieveError);
  const auto expiry = oauth.tokensExpireIn();
  const QList<QByteArray> malformed{
    "invalid JSON",
    R"({"expires_in":3600})",
    R"({"access_token":" ","expires_in":3600})",
    R"({"access_token":"replacement","refresh_token":"rotated"})",
    R"({"access_token":"replacement","expires_in":0})",
    R"({"access_token":"replacement","expires_in":-1})",
    R"({"access_token":"replacement","expires_in":1.5})",
    R"({"access_token":"replacement","expires_in":"3600"})",
  };
  for (const auto& reply : malformed) {
    received.clear();
    failures.clear();
    body = reply;
    oauth.refreshAccessToken();
    QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 1, 5000);
    QCOMPARE(tokens.size(), 1);
    QCOMPARE(oauth.accessToken(), QSL("fixture-access"));
    QCOMPARE(oauth.refreshToken(), QSL("fixture-refresh"));
    QCOMPARE(oauth.tokensExpireIn(), expiry);
  }

  received.clear();
  failures.clear();
  body = R"({"access_token":"replacement","expires_in":7200})";
  oauth.refreshAccessToken();
  QTRY_COMPARE_WITH_TIMEOUT(tokens.size(), 2, 5000);
  const QUrlQuery refresh(QString::fromUtf8(received.mid(received.indexOf("\r\n\r\n") + 4)));
  QVERIFY(!refresh.hasQueryItem(QSL("code_verifier")));
  QCOMPARE(oauth.accessToken(), QSL("replacement"));
  QCOMPARE(oauth.refreshToken(), QSL("fixture-refresh"));
  QCOMPARE(tokens.last().at(1).toString(), QSL("fixture-refresh"));
  QVERIFY(failures.isEmpty());
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        directory.path(),
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-oauth-loopback-test"), argc, argv, arguments);
  qApp->settings()->setValue(GROUP(Proxy), Proxy::Type, int(QNetworkProxy::NoProxy));
  TestOAuthLoopback tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "test_oauthloopback.moc"
