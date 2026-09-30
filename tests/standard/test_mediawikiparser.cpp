// For license of this file, see <project-root-folder>/LICENSE.md.

#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedfetchexception.h>
#include <src/parsers/mediawikiparser.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QUrlQuery>

// FeedParser::messages() is implemented inside the standard plugin and is not exported.
Q_NEVER_INLINE QList<Message> messagesViaBase(FeedParser* parser) {
  return parser->messages();
}

class TestMediaWikiParser : public QObject {
    Q_OBJECT

  private slots:
    void categoryHtmlUsesEditUri();
    void categoryHtmlUsesMinifiedBooleans_data();
    void categoryHtmlUsesMinifiedBooleans();
    void minifiedConfigPreservesStrings();
    void pageConfigRejectsOtherJavascript_data();
    void pageConfigRejectsOtherJavascript();
    void searchHtmlDecodesFormQuery();
    void categoryContinuationAndFullHtml();
    void continuationFailureFailsWholeFeed();
    void searchCatalogAndFullHtml();
    void directApiDiscovery();
    void articleApiErrorFailsWholeFeed();
    void htmlGuessProbesPreparedSource();
    void apiRateLimitFailsWholeFeed();
    void limitsArticlesAndPacesRequests();
};

void TestMediaWikiParser::categoryHtmlUsesEditUri() {
  const QByteArray html = R"(<html><head><meta content="MediaWiki 1.47" name="generator">
    <link href="//example.org/w/api.php?action=rsd" rel="EditURI">
    <script>RLCONF={"wgNamespaceNumber":14,"wgPageName":"Kategorie:Věda","wgSiteName":"Example Wiki"};</script>
    </head></html>)";
  NetworkResult result;
  result.m_url = QUrl(QSL("https://example.org/wiki/Kategorie:V%C4%9Bda"));
  MediaWikiParser parser({});
  auto guessed = parser.guessFeed(html, result);

  QVERIFY(guessed.m_feed != nullptr);
  const QUrl source(guessed.m_feed->source());
  QCOMPARE(source.path(), QSL("/w/api.php"));
  QCOMPARE(QUrlQuery(source).queryItemValue(QSL("list")), QSL("categorymembers"));
  QCOMPARE(QUrlQuery(source).queryItemValue(QSL("cmtitle")), QSL("Kategorie:Věda"));
  delete guessed.m_feed;
}

void TestMediaWikiParser::categoryHtmlUsesMinifiedBooleans_data() {
  QTest::addColumn<QByteArray>("assignment");
  QTest::addColumn<QByteArray>("ending");
  QTest::newRow("gentoo-rlconf") << QByteArray("RLCONF=") << QByteArray(";");
  QTest::newRow("window-rlconf") << QByteArray("window.RLCONF = ") << QByteArray(";");
  QTest::newRow("mw-config") << QByteArray("mw.config.set(") << QByteArray(");");
}

void TestMediaWikiParser::categoryHtmlUsesMinifiedBooleans() {
  QFETCH(QByteArray, assignment);
  QFETCH(QByteArray, ending);
  // Gentoo's MediaWiki 1.35 emits !0/!1 and advertises /api.php through EditURI.
  const QByteArray html = QByteArray(R"(<meta name="generator" content="MediaWiki 1.35.14">
    <link rel="EditURI" href="https://wiki.gentoo.org/api.php?action=rsd"><script>)") +
                          assignment +
                          QByteArray(R"({"wgBreakFrames":!1,"wgCanonicalSpecialPageName":!1,
    "wgNamespaceNumber":14,"wgPageName":"Category:Software","wgIsArticle":!0,
    "wgIsRedirect": !1 ,"nested":{"values":[!0, !1]},"last":!0})") +
                          ending + QByteArray("</script>");
  NetworkResult result;
  result.m_url = QUrl(QSL("https://wiki.gentoo.org/wiki/Category:Software"));
  int probes = 0;
  QUrl probed_url;
  MediaWikiParser parser({}, {}, [&](const QUrl& request) {
    ++probes;
    probed_url = request;
    return QByteArray(R"({"query":{"categorymembers":[]}})");
  });
  auto guessed = parser.guessFeed(html, result);

  QCOMPARE(probes, 1);
  QCOMPARE(probed_url.path(), QSL("/api.php"));
  QVERIFY(guessed.m_feed != nullptr);
  const QUrl source(guessed.m_feed->source());
  QCOMPARE(source.host(), QSL("wiki.gentoo.org"));
  QCOMPARE(source.path(), QSL("/api.php"));
  QCOMPARE(QUrlQuery(source).queryItemValue(QSL("cmtitle")), QSL("Category:Software"));
  delete guessed.m_feed;
}

void TestMediaWikiParser::minifiedConfigPreservesStrings() {
  const QByteArray html = R"(<meta name="generator" content="MediaWiki 1.35">
    <link rel="EditURI" href="/api.php?action=rsd"><script>
    RLCONF={"wgBreakFrames":!1,"wgNamespaceNumber":14,
    "wgPageName":"Category:Software !0 !1 \"quoted\" \\path","last":!0};</script>)";
  NetworkResult result;
  result.m_url = QUrl(QSL("https://example.org/wiki/Category:Software"));
  MediaWikiParser parser({});
  auto guessed = parser.guessFeed(html, result);

  QVERIFY(guessed.m_feed != nullptr);
  QCOMPARE(QUrlQuery(QUrl(guessed.m_feed->source())).queryItemValue(QSL("cmtitle"), QUrl::FullyDecoded),
           QSL(R"(Category:Software !0 !1 "quoted" \path)"));
  delete guessed.m_feed;
}

void TestMediaWikiParser::pageConfigRejectsOtherJavascript_data() {
  QTest::addColumn<QByteArray>("expression");
  QTest::newRow("other-number") << QByteArray("!2");
  QTest::newRow("longer-number") << QByteArray("!01");
  QTest::newRow("fraction") << QByteArray("!0.5");
  QTest::newRow("exponent") << QByteArray("!0e1");
  QTest::newRow("double-negation") << QByteArray("!!0");
  QTest::newRow("expression") << QByteArray("!0 && true");
  QTest::newRow("function-call") << QByteArray("getConfig()");
}

void TestMediaWikiParser::pageConfigRejectsOtherJavascript() {
  QFETCH(QByteArray, expression);
  const QByteArray html = QByteArray(R"(<meta name="generator" content="MediaWiki 1.35">
    <link rel="EditURI" href="/api.php?action=rsd"><script>RLCONF={"wgBreakFrames":)") +
                          expression +
                          QByteArray(R"(,"wgNamespaceNumber":14,"wgPageName":"Category:Software"};</script>)");
  NetworkResult result;
  result.m_url = QUrl(QSL("https://example.org/wiki/Category:Software"));
  MediaWikiParser parser({});

  QVERIFY_EXCEPTION_THROWN(parser.guessFeed(html, result), ApplicationException);
}

void TestMediaWikiParser::searchHtmlDecodesFormQuery() {
  const QByteArray html = R"(<html><head><meta name="generator" content="MediaWiki 1.47">
    <link rel="EditURI" href="/w/api.php?action=rsd">
    <script>window.RLCONF={"wgNamespaceNumber":-1,"wgCanonicalSpecialPageName":"Search","wgSiteName":"Example Wiki"};</script>
    </head></html>)";
  NetworkResult result;
  result.m_url = QUrl(QSL("https://example.org/w/index.php?title=Special:Search&search=quantum+computing&fulltext=1"));
  MediaWikiParser parser({});
  auto guessed = parser.guessFeed(html, result);

  QVERIFY(guessed.m_feed != nullptr);
  const QUrl source(guessed.m_feed->source());
  QCOMPARE(QUrlQuery(source).queryItemValue(QSL("list")), QSL("search"));
  QCOMPARE(QUrlQuery(source).queryItemValue(QSL("srsearch")), QSL("quantum computing"));
  delete guessed.m_feed;

  result.m_url = QUrl(QSL("https://example.org/w/index.php?title=Special:Search&search=C%2B%2B+%CE%BB&fulltext=1"));
  guessed = parser.guessFeed(html, result);
  QCOMPARE(QUrlQuery(QUrl(guessed.m_feed->source())).queryItemValue(QSL("srsearch")), QSL("C++ λ"));
  delete guessed.m_feed;
}

void TestMediaWikiParser::categoryContinuationAndFullHtml() {
  const QUrl source(QSL("https://example.org/w/"
                        "api.php?action=query&format=json&formatversion=2&list=categorymembers&cmtitle=Category%3ATest&"
                        "cmnamespace=0&cmprop=ids%7Ctitle%7Ctimestamp&cmsort=timestamp&cmdir=desc&cmlimit=5"));
  const QString first = QSL(
    R"({"continue":{"cmcontinue":"next","continue":"-||"},"query":{"categorymembers":[{"pageid":11,"title":"First","timestamp":"2026-09-24T08:00:00Z"}]}})");
  int catalog_requests = 0;
  int article_requests = 0;
  int pauses = 0;
  MediaWikiParser parser(
    first,
    source,
    [&](const QUrl& request) {
      const QUrlQuery query(request);

      if (query.queryItemValue(QSL("action")) == QSL("parse")) {
        ++article_requests;
        const QString id = query.queryItemValue(QSL("pageid"));
        return QSL(R"({"parse":{"pageid":%1,"text":"<a href='/wiki/Target'>Article %1</a>"}})").arg(id).toUtf8();
      }

      ++catalog_requests;
      return QByteArray(
        R"({"query":{"categorymembers":[{"pageid":11,"title":"First","timestamp":"2026-09-24T08:00:00Z"},{"pageid":12,"title":"Second","timestamp":"2026-09-24T07:00:00Z"}]}})");
    },
    [&]() {
      ++pauses;
    });

  const QList<Message> messages = messagesViaBase(&parser);
  QCOMPARE(messages.size(), 2);
  QCOMPARE(catalog_requests, 1);
  QCOMPARE(article_requests, 2);
  QCOMPARE(pauses, 3);
  QCOMPARE(messages.at(0).m_customId, QSL("11"));
  QCOMPARE(messages.at(1).m_customId, QSL("12"));
  QVERIFY(messages.at(0).m_contents.contains(QSL("https://example.org/wiki/Target")));
  QCOMPARE(messages.at(0).m_created.toUTC().toString(Qt::ISODate), QSL("2026-09-24T08:00:00Z"));
}

void TestMediaWikiParser::continuationFailureFailsWholeFeed() {
  const QUrl source(QSL("https://example.org/w/"
                        "api.php?action=query&format=json&formatversion=2&list=categorymembers&cmtitle=Category%3ATest&"
                        "cmnamespace=0&cmprop=ids%7Ctitle%7Ctimestamp&cmsort=timestamp&cmdir=desc&cmlimit=5"));
  const QString first = QSL(
    R"({"continue":{"cmcontinue":"next","continue":"-||"},"query":{"categorymembers":[{"pageid":11,"title":"First","timestamp":"2026-09-24T08:00:00Z"}]}})");
  int requests = 0;
  MediaWikiParser parser(
    first,
    source,
    [&](const QUrl&) {
      ++requests;
      return QByteArray("not JSON");
    },
    []() {});

  QVERIFY_EXCEPTION_THROWN(messagesViaBase(&parser), FeedFetchException);
  QCOMPARE(requests, 1);
}

void TestMediaWikiParser::searchCatalogAndFullHtml() {
  const QUrl source(QSL("https://example.org/w/"
                        "api.php?action=query&format=json&formatversion=2&list=search&srsearch=quantum%20computing&"
                        "srnamespace=0&srsort=last_edit_desc&srprop=snippet%7Ctimestamp%7Cwordcount&srlimit=5"));
  const QString data = QSL(
    R"({"query":{"search":[{"pageid":27,"title":"Quantum computing","timestamp":"2026-09-24T09:00:00Z","snippet":"quantum <span class='searchmatch'>computing</span>"}]}})");
  MediaWikiParser parser(
    data,
    source,
    [](const QUrl&) {
      return QByteArray(R"({"parse":{"pageid":27,"text":"<p>Full article</p>"}})");
    },
    []() {});

  const QList<Message> messages = messagesViaBase(&parser);
  QCOMPARE(messages.size(), 1);
  QCOMPARE(messages.first().m_customId, QSL("27"));
  QCOMPARE(messages.first().m_title, QSL("Quantum computing"));
  QCOMPARE(messages.first().m_contents, QSL("<p>Full article</p>"));
  QCOMPARE(messages.first().m_created.toUTC().toString(Qt::ISODate), QSL("2026-09-24T09:00:00Z"));
}

void TestMediaWikiParser::directApiDiscovery() {
  const QUrl source(QSL("https://example.org/w/"
                        "api.php?action=query&format=json&formatversion=2&list=search&srsearch=quantum%20computing&"
                        "srnamespace=0&srsort=last_edit_desc&srprop=snippet%7Ctimestamp%7Cwordcount&srlimit=5"));
  const QByteArray data(R"({"query":{"search":[]}})");
  MediaWikiParser parser({});
  const QList<StandardFeed*> feeds = parser.discoverFeeds(nullptr, source, false, {{data, source}});

  QCOMPARE(feeds.size(), 1);
  QCOMPARE(feeds.first()->source(), source.toString(QUrl::FullyEncoded));
  qDeleteAll(feeds);

  const QList<StandardFeed*> invalid =
    parser.discoverFeeds(nullptr, source, false, {{QByteArray(R"({"query":{"categorymembers":[]}})"), source}});
  QVERIFY(invalid.isEmpty());
}

void TestMediaWikiParser::articleApiErrorFailsWholeFeed() {
  const QUrl source(QSL("https://example.org/w/"
                        "api.php?action=query&format=json&formatversion=2&list=search&srsearch=test&srnamespace=0&"
                        "srsort=last_edit_desc&srprop=snippet%7Ctimestamp%7Cwordcount&srlimit=5"));
  const QString data = QSL(
    R"({"query":{"search":[{"pageid":27,"title":"Article","timestamp":"2026-09-24T09:00:00Z","snippet":"A <span class='searchmatch'>match</span> &amp; result"}]}})");
  int requests = 0;
  MediaWikiParser parser(
    data,
    source,
    [&](const QUrl&) {
      ++requests;
      return QByteArray(R"({"error":{"code":"missingtitle","info":"Page unavailable"}})");
    },
    []() {});

  QVERIFY_EXCEPTION_THROWN(messagesViaBase(&parser), FeedFetchException);
  QCOMPARE(requests, 1);
}

void TestMediaWikiParser::htmlGuessProbesPreparedSource() {
  const QByteArray html = R"(<meta name="generator" content="MediaWiki 1.47">
    <link rel="EditURI" href="/w/api.php?action=rsd">
    <script>RLCONF={"wgNamespaceNumber":14,"wgPageName":"Category:Test"};</script>)";
  NetworkResult result;
  result.m_url = QUrl(QSL("https://example.org/wiki/Category:Test"));
  int probes = 0;
  QString probed_list;
  MediaWikiParser parser({}, {}, [&](const QUrl& request) {
    ++probes;
    probed_list = QUrlQuery(request).queryItemValue(QSL("list"));
    return QByteArray(R"({"query":{"categorymembers":[]}})");
  });

  auto guessed = parser.guessFeed(html, result);
  QCOMPARE(probes, 1);
  QCOMPARE(probed_list, QSL("categorymembers"));
  QCOMPARE(QUrl(guessed.m_feed->source()).path(), QSL("/w/api.php"));
  delete guessed.m_feed;
}

void TestMediaWikiParser::apiRateLimitFailsWholeFeed() {
  const QUrl source(QSL("https://example.org/w/"
                        "api.php?action=query&format=json&formatversion=2&list=categorymembers&cmtitle=Category%3ATest&"
                        "cmnamespace=0&cmprop=ids%7Ctitle%7Ctimestamp&cmsort=timestamp&cmdir=desc&cmlimit=5"));
  const QString data = QSL(
    R"({"query":{"categorymembers":[{"pageid":379406,"title":"Great Soviet Encyclopedia","timestamp":"2025-04-19T01:34:00Z"},{"pageid":379407,"title":"Another article","timestamp":"2025-04-19T01:33:00Z"}]}})");
  int parse_requests = 0;
  int page_requests = 0;
  MediaWikiParser parser(
    data,
    source,
    [&](const QUrl& request) {
      const QUrlQuery query(request);

      if (query.queryItemValue(QSL("action")) == QSL("parse")) {
        ++parse_requests;
        return QByteArray("You are making too many requests to the API.");
      }

      ++page_requests;
      return QByteArray("<html><body>This page must not be requested.</body></html>");
    },
    []() {});

  QVERIFY_EXCEPTION_THROWN(messagesViaBase(&parser), FeedFetchException);
  QCOMPARE(parse_requests, 1);
  QCOMPARE(page_requests, 0);
}

void TestMediaWikiParser::limitsArticlesAndPacesRequests() {
  const QUrl source(QSL("https://example.org/w/api.php?action=query&format=json&list=search&srsearch=test"));
  const QString data = QSL(R"({"query":{"search":[
    {"pageid":1,"title":"First","timestamp":"2026-09-24T10:00:00Z"},
    {"pageid":2,"title":"Second","timestamp":"2026-09-24T09:00:00Z"},
    {"pageid":3,"title":"Third","timestamp":"2026-09-24T08:00:00Z"},
    {"pageid":4,"title":"Fourth","timestamp":"2026-09-24T07:00:00Z"},
    {"pageid":5,"title":"Fifth","timestamp":"2026-09-24T06:00:00Z"},
    {"pageid":6,"title":"Sixth","timestamp":"2026-09-24T05:00:00Z"}
  ]}})");
  int article_requests = 0;
  int pauses = 0;
  MediaWikiParser parser(
    data,
    source,
    [&](const QUrl& request) {
      ++article_requests;
      const QString id = QUrlQuery(request).queryItemValue(QSL("pageid"));
      return QSL(R"({"parse":{"pageid":%1,"text":"<p>Article %1</p>"}})").arg(id).toUtf8();
    },
    [&]() {
      ++pauses;
    });

  const QList<Message> messages = messagesViaBase(&parser);
  QCOMPARE(messages.size(), 5);
  QCOMPARE(article_requests, 5);
  QCOMPARE(pauses, 5);
  QCOMPARE(messages.first().m_customId, QSL("1"));
  QCOMPARE(messages.last().m_customId, QSL("5"));
}

QTEST_MAIN(TestMediaWikiParser)

#include "test_mediawikiparser.moc"
