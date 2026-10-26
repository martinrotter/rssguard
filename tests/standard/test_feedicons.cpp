// For license of this file, see <project-root-folder>/LICENSE.md.

#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/gui/dialogs/formprogressworker.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <src/gui/feediconselection.h>
#include <src/parsers/feedparser.h>

#include <QBuffer>
#include <QDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QtConcurrentRun>

class TestFeedIcons : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void distinctIconUrlsHaveSeparateCacheEntries();
    void candidateOrderIsPreserved();
    void invalidDownloadsAreRetried();
    void failedDownloadsDoNotReusePreviousIcon();
    void localFilesAndScriptsWithoutCandidatesDoNotFetch();
    void fetchesIndividualSourcesInWorkers();
    void respectsSourceTimeout();
    void stagesIndividualIconsAndPreservesFailures();
    void sharedChoiceAndInvalidation();
    void cancelPreservesPreviousSelection();

  private:
    QString url(const QString& path) const {
      return QSL("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path);
    }

    QImage fetch(const QList<IconLocation>& locations, QHash<QByteArray, QImage>& cache) {
      return FeedParser::fetchFeedIcon(locations,
                                       StandardFeed::SourceType::Url,
                                       QSL("https://feeds.example.org/feed"),
                                       {{"X-Icon-Test", "present"}},
                                       QNetworkProxy(QNetworkProxy::NoProxy),
                                       &cache);
    }

    QTcpServer m_server;
    QHash<QString, QByteArray> m_responses;
    QByteArray m_lastRequest;
    QHash<QString, QByteArray> m_requestsByPath;
    int m_requests = 0;
};

void TestFeedIcons::initTestCase() {
  for (const auto& item : {qMakePair(QSL("/red"), Qt::red), qMakePair(QSL("/blue"), Qt::blue)}) {
    QImage image(4, 4, QImage::Format_ARGB32);
    image.fill(item.second);
    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(image.save(&buffer, "PNG"));
    m_responses.insert(item.first, buffer.data());
  }

  connect(&m_server, &QTcpServer::newConnection, this, [this]() {
    while (m_server.hasPendingConnections()) {
      QTcpSocket* socket = m_server.nextPendingConnection();
      connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      connect(socket, &QTcpSocket::readyRead, this, [this, socket, request = QByteArray()]() mutable {
        request += socket->readAll();
        if (!request.contains("\r\n\r\n")) {
          return;
        }

        ++m_requests;
        m_lastRequest = request;
        const QString path = QString::fromUtf8(request.split(' ').value(1));
        m_requestsByPath.insert(path, request);
        if (path == QSL("/slow")) {
          return;
        }
        const QByteArray status = m_responses.contains(path) ? "200 OK" : "404 Not Found";
        const QByteArray body = m_responses.value(path);
        socket->write("HTTP/1.1 " + status + "\r\nContent-Length: " + QByteArray::number(body.size()) +
                      "\r\nConnection: close\r\n\r\n" + body);
        socket->disconnectFromHost();
      });
    }
  });
  QVERIFY(m_server.listen(QHostAddress::LocalHost));
}

void TestFeedIcons::distinctIconUrlsHaveSeparateCacheEntries() {
  QHash<QByteArray, QImage> cache;
  const int before = m_requests;
  const QImage red = fetch({{url(QSL("/red")), true}}, cache);
  const QImage blue = fetch({{url(QSL("/blue")), true}}, cache);
  QCOMPARE(red.pixelColor(0, 0), QColor(Qt::red));
  QCOMPARE(blue.pixelColor(0, 0), QColor(Qt::blue));
  QCOMPARE(fetch({{url(QSL("/red")), true}}, cache), red);
  QCOMPARE(m_requests - before, 2);
  QCOMPARE(cache.size(), 2);
}

void TestFeedIcons::candidateOrderIsPreserved() {
  QHash<QByteArray, QImage> cache;
  const IconLocation red{url(QSL("/red")), true};
  const IconLocation blue{url(QSL("/blue")), true};
  QCOMPARE(fetch({red, blue}, cache).pixelColor(0, 0), QColor(Qt::red));
  QCOMPARE(fetch({blue, red}, cache).pixelColor(0, 0), QColor(Qt::blue));
}

void TestFeedIcons::invalidDownloadsAreRetried() {
  QHash<QByteArray, QImage> cache;
  const QList<IconLocation> locations{{url(QSL("/retry")), true}};
  const int before = m_requests;
  m_responses.insert(QSL("/retry"), "<html>Not an image</html>");
  QVERIFY(fetch(locations, cache).isNull());
  QVERIFY(cache.isEmpty());
  QVERIFY(m_lastRequest.toLower().contains("x-icon-test: present\r\n"));
  m_responses.insert(QSL("/retry"), m_responses.value(QSL("/blue")));
  QCOMPARE(fetch(locations, cache).pixelColor(0, 0), QColor(Qt::blue));
  QCOMPARE(m_requests - before, 2);
  QCOMPARE(cache.size(), 1);
}

void TestFeedIcons::failedDownloadsDoNotReusePreviousIcon() {
  QHash<QByteArray, QImage> cache;
  QVERIFY(!fetch({{url(QSL("/red")), true}}, cache).isNull());
  m_responses.insert(QSL("/invalid"), "Not an image");
  QVERIFY(fetch({{url(QSL("/invalid")), true}}, cache).isNull());
  QVERIFY(fetch({{url(QSL("/missing")), true}}, cache).isNull());
  QCOMPARE(cache.size(), 1);
  QCOMPARE(fetch({{url(QSL("/missing")), true}, {url(QSL("/blue")), true}}, cache).pixelColor(0, 0), QColor(Qt::blue));
}

void TestFeedIcons::localFilesAndScriptsWithoutCandidatesDoNotFetch() {
  QHash<QByteArray, QImage> cache;
  for (const auto type : {StandardFeed::SourceType::LocalFile, StandardFeed::SourceType::Script}) {
    QVERIFY(FeedParser::fetchFeedIcon({}, type, QSL("local-source"), {}, {}, &cache).isNull());
  }
  QVERIFY(cache.isEmpty());
}

void TestFeedIcons::fetchesIndividualSourcesInWorkers() {
  auto request = [this](const QString& name, const QString& image, const QString& user) {
    m_responses
      .insert(name,
              (QSL("<rss version='2.0'><channel><title>Feed</title><image><url>%1</url></image></channel></rss>")
                 .arg(url(image)))
                .toUtf8());
    FeedIconRequest result;
    result.m_source = url(name);
    result.m_protection = NetworkFactory::NetworkAuthentication::Basic;
    result.m_username = user;
    result.m_password = QSL("password");
    result.m_headers = {{"X-Feed", user.toUtf8()}};
    result.m_proxy = QNetworkProxy(QNetworkProxy::NoProxy);
    result.m_http2Status = NetworkFactory::Http2Status::Disabled;
    result.m_ignoreCookies = true;
    result.m_timeout = 2000;
    return result;
  };
  const auto first = request(QSL("/feed-one"), QSL("/red"), QSL("alice"));
  const auto second = request(QSL("/feed-two"), QSL("/blue"), QSL("bob"));
  auto one = QtConcurrent::run([first]() {
    return FeedParser::fetchFeedIconForSource(first, nullptr);
  });
  auto two = QtConcurrent::run([second]() {
    return FeedParser::fetchFeedIconForSource(second, nullptr);
  });
  QTRY_VERIFY(one.isFinished() && two.isFinished());
  QCOMPARE(one.result().pixelColor(0, 0), QColor(Qt::red));
  QCOMPARE(two.result().pixelColor(0, 0), QColor(Qt::blue));
  QVERIFY(m_requestsByPath.value(QSL("/feed-one"))
            .contains(QByteArray("Authorization: Basic ").append(QByteArray("alice:password").toBase64())));
  QVERIFY(m_requestsByPath.value(QSL("/feed-two"))
            .contains(QByteArray("Authorization: Basic ").append(QByteArray("bob:password").toBase64())));
  QVERIFY(m_requestsByPath.value(QSL("/feed-one")).toLower().contains("x-feed: alice"));
  QVERIFY(m_requestsByPath.value(QSL("/feed-two")).toLower().contains("x-feed: bob"));
}

void TestFeedIcons::respectsSourceTimeout() {
  FeedIconRequest request;
  request.m_source = url(QSL("/slow"));
  request.m_proxy = QNetworkProxy(QNetworkProxy::NoProxy);
  request.m_timeout = 100;
  auto future = QtConcurrent::run([request]() {
    try {
      FeedParser::fetchFeedIconForSource(request, nullptr);
      return false;
    }
    catch (const ApplicationException&) {
      return true;
    }
  });
  QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 3000);
  QVERIFY(future.result());
}

void TestFeedIcons::stagesIndividualIconsAndPreservesFailures() {
  QPixmap red(4, 4);
  red.fill(Qt::red);
  QPixmap blue(4, 4);
  blue.fill(Qt::blue);
  FeedIconSelection selection;
  selection.selectIndividual({{0, QIcon(red)}, {2, QIcon(blue)}});
  QCOMPARE(selection.iconForFeed(0, QIcon(blue), true)->pixmap(4, 4).toImage(), red.toImage());
  QCOMPARE(selection.iconForFeed(2, QIcon(red), true)->pixmap(4, 4).toImage(), blue.toImage());
  QVERIFY(!selection.iconForFeed(1, QIcon(red), true).has_value());
  QVERIFY(!selection.iconForFeed(0, QIcon(red), false).has_value());
  QVERIFY(selection.iconForFeed(0, QIcon(red), true).has_value());
}

void TestFeedIcons::sharedChoiceAndInvalidation() {
  QPixmap image(4, 4);
  image.fill(Qt::blue);
  FeedIconSelection selection;
  selection.selectIndividual({{0, QIcon(image)}});
  selection.invalidate();
  QVERIFY(selection.isIndividual());
  QVERIFY(!selection.iconForFeed(0, QIcon(image), true).has_value());
  selection.selectShared();
  QCOMPARE(selection.iconForFeed(0, QIcon(image), true)->pixmap(4, 4).toImage(), image.toImage());
  QCOMPARE(selection.iconForFeed(1, QIcon(image), true)->pixmap(4, 4).toImage(), image.toImage());
  // Choosing the default icon is an intentional change, unlike a failed fetch.
  QVERIFY(selection.iconForFeed(1, {}, true).has_value());
  QVERIFY(selection.iconForFeed(1, {}, true)->isNull());
  QVERIFY(!selection.iconForFeed(1, {}, false).has_value());
}

void TestFeedIcons::cancelPreservesPreviousSelection() {
  QPixmap image(4, 4);
  image.fill(Qt::red);
  FeedIconSelection selection;
  selection.selectIndividual({{0, QIcon(image)}});
  FormProgressWorker progress;
  QTimer::singleShot(0, &progress, [&progress]() {
    QMetaObject::invokeMethod(&progress, "requestCancellation", Qt::DirectConnection);
  });
  const int result = progress.doWork<int>(
    QSL("Fetching icons"),
    true,
    {1, 2, 3},
    [](int) {
      QThread::msleep(50);
    },
    [](int count) {
      return QString::number(count);
    });
  if (result == QDialog::Accepted) {
    selection.selectIndividual({});
  }
  QCOMPARE(result, int(QDialog::Rejected));
  QCOMPARE(selection.iconForFeed(0, {}, true)->pixmap(4, 4).toImage(), image.toImage());
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
  Application application(QSL("rssguard-feed-icons-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  TestFeedIcons tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_feedicons.moc"
