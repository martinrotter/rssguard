// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "network-web/cookiejar.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include <QWebEngineCookieStore>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#endif

#include <QDir>
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
    void preservesBrowserCookiesWhenSettingsAreReapplied();

  private:
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

    QTcpServer m_server;
    QByteArray m_body;
};

void TestCookies::initTestCase() {
  connect(&m_server, &QTcpServer::newConnection, this, [this]() {
    while (m_server.hasPendingConnections()) {
      auto* socket = m_server.nextPendingConnection();
      connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      connect(socket, &QTcpSocket::readyRead, this, [socket, request = QByteArray()]() mutable {
        request += socket->readAll();
        if (!request.contains("\r\n\r\n")) {
          return;
        }

        const QByteArray path = request.split(' ').value(1);
        QByteArray cookies;
        for (const auto& line : request.split('\n')) {
          if (line.toLower().startsWith("cookie:")) {
            cookies = line.mid(7).trimmed();
          }
        }
        QByteArray response = "HTTP/1.1 200 OK\r\n";
        if (path == "/seed") {
          response += "Set-Cookie: stored=login; Path=/\r\n";
        }
        else if (path == "/redirect") {
          response = "HTTP/1.1 302 Found\r\nLocation: /echo\r\nSet-Cookie: redirected=yes; Path=/\r\n";
        }
        socket->write(response + "Content-Length: " + QByteArray::number(cookies.size()) +
                      "\r\nConnection: close\r\n\r\n" + cookies);
        socket->disconnectFromHost();
      });
    }
  });
  QVERIFY(m_server.listen(QHostAddress::LocalHost));
}

void TestCookies::init() {
  setGlobalIgnoring(false);
  qApp->web()->cookieJar()->clearCookies();
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
