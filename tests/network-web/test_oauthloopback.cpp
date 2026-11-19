// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "network-web/httpserver.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"

#include <QNetworkProxy>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

class TestServer : public HttpServer {
  protected:
    void answerClient(QTcpSocket*, const HttpRequest&) override {}
};

class TokenEndpoint : public QTcpServer {
  public:
    TokenEndpoint() {
      connect(this, &QTcpServer::newConnection, this, [this]() {
        auto* socket = nextPendingConnection();
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        connect(socket, &QTcpSocket::readyRead, socket, [this, socket, request = QByteArray()]() mutable {
          request += socket->readAll();
          const auto headers_end = request.indexOf("\r\n\r\n");
          if (headers_end < 0) {
            return;
          }
          int content_length = -1;
          for (const auto& header : request.left(headers_end).split('\n')) {
            if (header.toLower().startsWith("content-length:")) {
              content_length = header.mid(15).trimmed().toInt();
            }
          }
          if (content_length < 0 || request.size() - headers_end - 4 < content_length) {
            return;
          }
          m_request = request;
          if (!m_stall) {
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                          QByteArray::number(m_body.size()) + "\r\n\r\n" + m_body);
            socket->disconnectFromHost();
          }
        });
      });
    }

    QString tokenUrl() const {
      return QSL("http://127.0.0.1:%1/token").arg(serverPort());
    }

    QUrlQuery form() const {
      return QUrlQuery(QString::fromUtf8(m_request.mid(m_request.indexOf("\r\n\r\n") + 4)));
    }

    QByteArray m_request;
    QByteArray m_body = R"({"access_token":"fixture-access","refresh_token":"fixture-refresh","expires_in":3600})";
    bool m_stall = false;
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
    void tokenRequestTimeout_data();
    void tokenRequestTimeout();
    void tokenRequestProxyCanBeReset();
};

void TestOAuthLoopback::tokenExchange() {
  QFETCH(bool, pkce);
  TokenEndpoint endpoint;
  QVERIFY(endpoint.listen(QHostAddress::LocalHost));
  OAuth2Service oauth(QSL("https://example.invalid/authorize"),
                      endpoint.tokenUrl(),
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
  const auto form = endpoint.form();
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
    failures.clear();
    endpoint.m_body = reply;
    oauth.refreshAccessToken();
    QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 1, 5000);
    QCOMPARE(tokens.size(), 1);
    QCOMPARE(oauth.accessToken(), QSL("fixture-access"));
    QCOMPARE(oauth.refreshToken(), QSL("fixture-refresh"));
    QCOMPARE(oauth.tokensExpireIn(), expiry);
  }

  failures.clear();
  endpoint.m_body = R"({"access_token":"replacement","expires_in":7200})";
  oauth.refreshAccessToken();
  QTRY_COMPARE_WITH_TIMEOUT(tokens.size(), 2, 5000);
  const auto refresh = endpoint.form();
  QVERIFY(!refresh.hasQueryItem(QSL("code_verifier")));
  QCOMPARE(oauth.accessToken(), QSL("replacement"));
  QCOMPARE(oauth.refreshToken(), QSL("fixture-refresh"));
  QCOMPARE(tokens.last().at(1).toString(), QSL("fixture-refresh"));
  QVERIFY(failures.isEmpty());
}

void TestOAuthLoopback::tokenRequestTimeout_data() {
  QTest::addColumn<bool>("refresh");
  QTest::newRow("exchange") << false;
  QTest::newRow("refresh") << true;
}

void TestOAuthLoopback::tokenRequestTimeout() {
  QFETCH(bool, refresh);
  TokenEndpoint endpoint;
  QVERIFY(endpoint.listen(QHostAddress::LocalHost));
  endpoint.m_stall = true;
  OAuth2Service oauth(QSL("https://example.invalid/authorize"), endpoint.tokenUrl(), QSL("fixture-client"), {}, {});
  oauth.setTokenRequestTimeout(500);
  const auto expiry = QDateTime::currentDateTime().addSecs(3600);
  oauth.setAccessToken(QSL("previous-access"));
  oauth.setRefreshToken(QSL("previous-refresh"));
  oauth.setTokensExpireIn(expiry);
  QSignalSpy tokens(&oauth, &OAuth2Service::tokensRetrieved);
  QSignalSpy failures(&oauth, &OAuth2Service::tokensRetrieveError);
  const auto request_tokens = [&](const QString& auth_code) {
    if (refresh) {
      oauth.refreshAccessToken();
    }
    else {
      oauth.retrieveAccessToken(auth_code);
    }
  };

  request_tokens(QSL("first-code"));
  QTRY_VERIFY_WITH_TIMEOUT(!endpoint.m_request.isEmpty(), 5000);
  QCOMPARE(endpoint.form().queryItemValue(QSL("grant_type")), refresh ? QSL("refresh_token") : QSL("authorization_code"));
  QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 1, 5000);
  QCOMPARE(failures.first().at(1).toString(), NetworkFactory::networkErrorText(QNetworkReply::TimeoutError));
  QVERIFY(tokens.isEmpty());
  QCOMPARE(oauth.accessToken(), QSL("previous-access"));
  QCOMPARE(oauth.refreshToken(), QSL("previous-refresh"));
  QCOMPARE(oauth.tokensExpireIn(), expiry);

  endpoint.m_request.clear();
  endpoint.m_stall = false;
  oauth.setTokenRequestTimeout(0);
  request_tokens(QSL("retry-code"));
  QTRY_COMPARE_WITH_TIMEOUT(tokens.size(), 1, 5000);
  QCOMPARE(failures.size(), 1);
  QCOMPARE(oauth.accessToken(), QSL("fixture-access"));
  QCOMPARE(oauth.refreshToken(), QSL("fixture-refresh"));
  if (refresh) {
    QCOMPARE(endpoint.form().queryItemValue(QSL("refresh_token")), QSL("previous-refresh"));
  }
  else {
    QCOMPARE(endpoint.form().queryItemValue(QSL("code")), QSL("retry-code"));
  }
}

void TestOAuthLoopback::tokenRequestProxyCanBeReset() {
  TokenEndpoint origin;
  TokenEndpoint proxy;
  QVERIFY(origin.listen(QHostAddress::LocalHost));
  QVERIFY(proxy.listen(QHostAddress::LocalHost));
  OAuth2Service oauth(QSL("https://example.invalid/authorize"), origin.tokenUrl(), QSL("fixture-client"), {}, {});
  QSignalSpy tokens(&oauth, &OAuth2Service::tokensRetrieved);
  QSignalSpy failures(&oauth, &OAuth2Service::tokensRetrieveError);

  oauth.setTokenRequestProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QSL("127.0.0.1"), proxy.serverPort()));
  oauth.refreshAccessToken(QSL("proxy-refresh"));
  QTRY_COMPARE_WITH_TIMEOUT(tokens.size(), 1, 5000);
  QVERIFY(origin.m_request.isEmpty());
  QCOMPARE(proxy.m_request.split('\n').first().trimmed(), "POST " + origin.tokenUrl().toUtf8() + " HTTP/1.1");
  QCOMPARE(proxy.form().queryItemValue(QSL("refresh_token")), QSL("proxy-refresh"));

  oauth.setTokenRequestProxy(QNetworkProxy::DefaultProxy);
  oauth.refreshAccessToken(QSL("direct-refresh"));
  QTRY_COMPARE_WITH_TIMEOUT(tokens.size(), 2, 5000);
  QCOMPARE(origin.m_request.split('\n').first().trimmed(), QByteArray("POST /token HTTP/1.1"));
  QCOMPARE(origin.form().queryItemValue(QSL("refresh_token")), QSL("direct-refresh"));
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
