// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "network-web/cookiejar.h"
#include "network-web/downloader.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <utility>

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include <QWebEngineCookieStore>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#endif

#include <QDir>
#include <QMap>
#include <QNetworkCookie>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

class TestCookies : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void respectsDownloadPolicy_data();
    void respectsDownloadPolicy();
    void preservesExplicitCookieHeader();
    void completesLocalReplyOnce();
    void sendsCustomMethod_data();
    void sendsCustomMethod();
    void returnsRedirectResponse_data();
    void returnsRedirectResponse();
    void rejectsInvalidMethods_data();
    void rejectsInvalidMethods();
    void redirectLimitReturnsError();
    void preservesBrowserCookiesWhenSettingsAreReapplied();

  private:
    struct Request {
        QByteArray m_method;
        QByteArray m_path;
        QMap<QByteArray, QByteArray> m_headers;
        QByteArray m_body;
    };

    QString url(const QString& path) const {
      return QSL("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path);
    }

    NetworkResult fetch(const QString& path,
                        bool ignore_cookies = false,
                        const QList<QPair<QByteArray, QByteArray>>& headers = {}) {
      return NetworkFactory::performNetworkOperation(url(path),
                                                     3000,
                                                     QByteArray(),
                                                     m_body,
                                                     QNetworkAccessManager::GetOperation,
                                                     headers,
                                                     false,
                                                     {},
                                                     {},
                                                     QNetworkProxy(QNetworkProxy::NoProxy),
                                                     NetworkFactory::Http2Status::Disabled,
                                                     ignore_cookies ? NetworkFactory::CookiePolicy::IgnoreCookies
                                                                    : NetworkFactory::CookiePolicy::UseSharedCookieJar);
    }

    void setGlobalIgnoring(bool ignore) {
      qApp->settings()->setValue(GROUP(Network), Network::IgnoreAllCookies, ignore);
      qApp->web()->cookieJar()->updateSettings();
    }

    NetworkResult request(const QString& path,
                          const QByteArray& body,
                          QNetworkAccessManager::Operation operation,
                          const NetworkFactory::HttpRequestOptions& options,
                          const QList<QPair<QByteArray, QByteArray>>& headers = {}) {
      return NetworkFactory::performNetworkOperation(url(path),
                                                     3000,
                                                     body,
                                                     m_body,
                                                     operation,
                                                     headers,
                                                     false,
                                                     {},
                                                     {},
                                                     QNetworkProxy::NoProxy,
                                                     NetworkFactory::Http2Status::Disabled,
                                                     NetworkFactory::CookiePolicy::IgnoreCookies,
                                                     options);
    }

    QTcpServer m_server;
    QList<Request> m_requests;
    QByteArray m_body;
};

void TestCookies::completesLocalReplyOnce() {
  const auto url = QSL("data:text/plain,local-reply");
  Downloader downloader;
  QSignalSpy completed(&downloader, &Downloader::completed);
  downloader.downloadFile(url);
  QTRY_COMPARE(completed.size(), 1);
  QCoreApplication::processEvents();
  QCOMPARE(completed.size(), 1);
  QCOMPARE(downloader.lastOutputData(), QByteArray("local-reply"));

  QByteArray body;
  const auto response = NetworkFactory::performNetworkOperation(
    url, 3000, QByteArray(), body, QNetworkAccessManager::GetOperation);
  QCOMPARE(response.m_networkError, QNetworkReply::NoError);
  QCOMPARE(response.m_url, QUrl(url));
  QCOMPARE(body, QByteArray("local-reply"));
}

void TestCookies::initTestCase() {
  connect(&m_server, &QTcpServer::newConnection, this, [this]() {
    while (m_server.hasPendingConnections()) {
      auto* socket = m_server.nextPendingConnection();
      connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      connect(socket, &QTcpSocket::readyRead, this, [this, socket, request = QByteArray()]() mutable {
        request += socket->readAll();
        const auto headers_end = request.indexOf("\r\n\r\n");
        if (headers_end < 0) {
          return;
        }

        const auto lines = request.left(headers_end).split('\n');
        const auto request_line = lines.first().split(' ');
        Request incoming{request_line.value(0), request_line.value(1), {}, {}};
        for (const auto& line : lines) {
          const auto separator = line.indexOf(':');
          if (separator > 0) {
            incoming.m_headers.insert(line.left(separator).toLower(), line.mid(separator + 1).trimmed());
          }
        }
        const auto content_length = incoming.m_headers.value("content-length").toInt();
        const auto body_start = headers_end + 4;
        if (request.size() - body_start < content_length) {
          return;
        }
        incoming.m_body = request.mid(body_start, content_length);
        m_requests.append(incoming);

        const auto& path = incoming.m_path;
        QByteArray response_body = incoming.m_headers.value("cookie");
        QByteArray response = "HTTP/1.1 200 OK\r\n";
        if (path == "/seed") {
          response += "Set-Cookie: stored=login; Path=/\r\n";
        }
        else if (path == "/redirect") {
          response = "HTTP/1.1 302 Found\r\nLocation: /echo\r\nSet-Cookie: redirected=yes; Path=/\r\n";
        }
        else if (path == "/redirect-method") {
          response = "HTTP/1.1 307 Temporary Redirect\r\nLocation: /method\r\nX-Response: original\r\n";
          response_body = "redirect body";
        }
        else if (path == "/redirect-loop") {
          response = "HTTP/1.1 307 Temporary Redirect\r\nLocation: /redirect-loop\r\n";
        }
        else if (path == "/method") {
          response_body = "updated";
        }
        socket->write(response + "Content-Length: " + QByteArray::number(response_body.size()) +
                      "\r\nConnection: close\r\n\r\n" + response_body);
        socket->disconnectFromHost();
      });
    }
  });
  QVERIFY(m_server.listen(QHostAddress::LocalHost));
}

void TestCookies::init() {
  m_requests.clear();
  setGlobalIgnoring(false);
  qApp->web()->cookieJar()->clearCookies();
}

void TestCookies::sendsCustomMethod_data() {
  QTest::addColumn<QString>("path");
  QTest::addColumn<int>("requests");
  QTest::newRow("direct") << QSL("/method") << 1;
  QTest::newRow("follow-307") << QSL("/redirect-method") << 2;
}

void TestCookies::sendsCustomMethod() {
  QFETCH(QString, path);
  QFETCH(int, requests);
  const QByteArray body(R"({"summary":"Updated"})");
  const auto response = request(path,
                                body,
                                QNetworkAccessManager::CustomOperation,
                                {"PATCH"},
                                {{"Content-Type", "application/json"}, {"If-Match", "\"fixture-etag\""}});
  QCOMPARE(response.m_networkError, QNetworkReply::NoError);
  QCOMPARE(response.m_httpCode, 200);
  QCOMPARE(response.m_url, QUrl(url(QSL("/method"))));
  QCOMPARE(m_body, QByteArray("updated"));
  QCOMPARE(m_requests.size(), requests);
  QCOMPARE(m_requests.last().m_path, QByteArray("/method"));
  for (const auto& received : std::as_const(m_requests)) {
    QCOMPARE(received.m_method, QByteArray("PATCH"));
    QCOMPARE(received.m_body, body);
    QCOMPARE(received.m_headers.value("content-type"), QByteArray("application/json"));
    QCOMPARE(received.m_headers.value("if-match"), QByteArray("\"fixture-etag\""));
  }
}

void TestCookies::returnsRedirectResponse_data() {
  QTest::addColumn<bool>("custom_method");
  QTest::newRow("get") << false;
  QTest::newRow("patch") << true;
}

void TestCookies::returnsRedirectResponse() {
  QFETCH(bool, custom_method);
  const auto path = QSL("/redirect-method");
  const auto operation = custom_method ? QNetworkAccessManager::CustomOperation : QNetworkAccessManager::GetOperation;
  const QByteArray method = custom_method ? "PATCH" : "GET";
  const auto response = request(path,
                                custom_method ? QByteArray("{}") : QByteArray(),
                                operation,
                                {custom_method ? method : QByteArray(), NetworkFactory::RedirectPolicy::ReturnResponse},
                                {{"Authorization", "Bearer fixture-token"}});
  QCOMPARE(response.m_networkError, QNetworkReply::NoError);
  QCOMPARE(response.m_httpCode, 307);
  QCOMPARE(response.m_url, QUrl(url(path)));
  QCOMPARE(response.m_headers.value(QSL("location")), QSL("/method"));
  QCOMPARE(response.m_headers.value(QSL("x-response")), QSL("original"));
  QCOMPARE(m_body, QByteArray("redirect body"));
  QCOMPARE(m_requests.size(), 1);
  QCOMPARE(m_requests.first().m_path, QByteArray("/redirect-method"));
  QCOMPARE(m_requests.first().m_method, method);
}

void TestCookies::rejectsInvalidMethods_data() {
  QTest::addColumn<QByteArray>("method");
  QTest::newRow("empty") << QByteArray();
  QTest::newRow("contains-whitespace") << QByteArray("PATCH GET");
}

void TestCookies::rejectsInvalidMethods() {
  QFETCH(QByteArray, method);
  const auto response = request(QSL("/method"), {}, QNetworkAccessManager::CustomOperation, {method});
  QCOMPARE(response.m_networkError, QNetworkReply::ProtocolInvalidOperationError);
  QCOMPARE(response.m_httpCode, 0);
  QVERIFY(m_body.isEmpty());
  QVERIFY(m_requests.isEmpty());
}

void TestCookies::redirectLimitReturnsError() {
  QCOMPARE(fetch(QSL("/redirect-loop")).m_networkError, QNetworkReply::TooManyRedirectsError);
}

void TestCookies::respectsDownloadPolicy_data() {
  QTest::addColumn<bool>("global_ignore");
  QTest::addColumn<bool>("feed_ignore");
  QTest::newRow("cookies-allowed") << false << false;
  QTest::newRow("feed-ignores-cookies") << false << true;
  QTest::newRow("global-ignores-cookies") << true << false;
  QTest::newRow("both-ignore-cookies") << true << true;
}

void TestCookies::respectsDownloadPolicy() {
  QFETCH(bool, global_ignore);
  QFETCH(bool, feed_ignore);
  QCOMPARE(fetch(QSL("/echo")).m_networkError, QNetworkReply::NoError);
  QVERIFY(m_body.isEmpty());
  QCOMPARE(fetch(QSL("/seed")).m_networkError, QNetworkReply::NoError);

  setGlobalIgnoring(global_ignore);
  QCOMPARE(fetch(QSL("/redirect"), feed_ignore).m_networkError, QNetworkReply::NoError);
  if (global_ignore || feed_ignore) {
    QVERIFY(m_body.isEmpty());
  }
  else {
    QVERIFY(m_body.contains("stored=login"));
    QVERIFY(m_body.contains("redirected=yes"));
  }

  // Ignoring a download must not retain its response cookies for later requests.
  setGlobalIgnoring(false);
  QCOMPARE(fetch(QSL("/echo")).m_networkError, QNetworkReply::NoError);
  QCOMPARE(m_body.contains("stored=login"), !global_ignore);
  QCOMPARE(m_body.contains("redirected=yes"), !global_ignore && !feed_ignore);
}

void TestCookies::preservesExplicitCookieHeader() {
  QCOMPARE(fetch(QSL("/echo"), false, {{"Cookie", "manual=login"}}).m_networkError, QNetworkReply::NoError);
  QCOMPARE(m_body, QByteArray("manual=login"));
}

void TestCookies::preservesBrowserCookiesWhenSettingsAreReapplied() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  QWebEnginePage page(qApp->web()->webEngineProfile());
  QSignalSpy loaded(&page, &QWebEnginePage::loadFinished);
  page.load(QUrl(url(QSL("/echo"))));
  QVERIFY(!loaded.isEmpty() || loaded.wait(10000));
  QVERIFY(loaded.last().at(0).toBool());

  auto* store = qApp->web()->webEngineProfile()->cookieStore();
  QSignalSpy added(store, &QWebEngineCookieStore::cookieAdded);
  QSignalSpy removed(store, &QWebEngineCookieStore::cookieRemoved);
  const auto hasCookie = [](const QSignalSpy& changes, const QByteArray& value) {
    for (const auto& change : changes) {
      const auto cookie = qvariant_cast<QNetworkCookie>(change.at(0));
      if (cookie.name() == "browser" && cookie.value() == value) {
        return true;
      }
    }
    return false;
  };
  QNetworkCookie cookie("browser", "before-enabling");
  cookie.setPath(QSL("/"));
  store->setCookie(cookie, QUrl(url(QSL("/"))));
  QTRY_VERIFY(hasCookie(added, "before-enabling"));

  setGlobalIgnoring(true);
  QTRY_VERIFY(hasCookie(removed, "before-enabling"));

  cookie.setValue("new-login");
  store->setCookie(cookie, QUrl(url(QSL("/"))));
  QTRY_VERIFY(hasCookie(added, "new-login"));
  setGlobalIgnoring(true);
  loaded.clear();
  page.load(QUrl(url(QSL("/echo"))));
  QVERIFY(!loaded.isEmpty() || loaded.wait(10000));
  QVERIFY(loaded.last().at(0).toBool());
  QString browser_cookies;
  bool received_text = false;
  page.toPlainText([&](const QString& text) {
    browser_cookies = text;
    received_text = true;
  });
  QTRY_VERIFY(received_text);
  QVERIFY(browser_cookies.contains(QSL("browser=new-login")));
  QCOMPARE(fetch(QSL("/echo")).m_networkError, QNetworkReply::NoError);
  QVERIFY(m_body.isEmpty());

  // Turning the option off and on again still clears browser cookies.
  setGlobalIgnoring(false);
  setGlobalIgnoring(true);
  QTRY_VERIFY(hasCookie(removed, "new-login"));
#else
  QSKIP("Browser cookies require WebEngine.");
#endif
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  const QString config = directory.path() + QDir::separator() + QSL(APP_CFG_PATH);
  QDir().mkpath(config);
  QSettings settings(config + QDir::separator() + QSL(APP_CFG_FILE), QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.sync();
  const QStringList arguments{QString::fromLocal8Bit(argv[0]),
                              QSL("--data"),
                              directory.path(),
                              QSL("--no-single-instance"),
                              QSL("--force-text-browser")};
  Application application(QSL("rssguard-cookies-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  TestCookies tests;
  const int result = QTest::qExec(&tests, argc, argv);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QTest::qWait(100);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  return result;
}

#include "test_cookies.moc"
