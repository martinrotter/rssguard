// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QTest>

class TestWebFactory : public QObject {
    Q_OBJECT

  private slots:
    void unescapesAllHtmlEntities();
    void unescapesOnlyNumericEntitiesAboveBoundary();
    void resolvesCookiePolicy_data();
    void resolvesCookiePolicy();
};

void TestWebFactory::unescapesAllHtmlEntities() {
  QString expected = QStringLiteral("A & B ");

  expected.append(QChar(0x0161));

  QCOMPARE(WebFactory::unescapeHtml(QStringLiteral("A &amp; B &#353;")), expected);
}

void TestWebFactory::unescapesOnlyNumericEntitiesAboveBoundary() {
  QString expected = QStringLiteral("&amp; &#127; ");

  expected.append(QChar(0x0080));
  expected.append(QLatin1Char(' '));
  expected.append(QChar(0x0161));
  expected.append(QLatin1Char(' '));
  expected.append(QChar(0x0161));
  expected.append(QLatin1Char(' '));
  expected.append(QChar(0x2013));
  expected.append(QStringLiteral(" &#60; &#xD800; &#x110000;"));

  QCOMPARE(WebFactory::
             unescapeHtml(QStringLiteral("&amp; &#127; &#128; &#353; &#x161; &#X2013; &#60; &#xD800; &#x110000;"),
                          char32_t{0x80}),
           expected);
}

void TestWebFactory::resolvesCookiePolicy_data() {
  QTest::addColumn<NetworkFactory::CookiePolicy>("requested");
  QTest::addColumn<bool>("ignore_all_cookies");
  QTest::addColumn<NetworkFactory::CookiePolicy>("expected");

  using CookiePolicy = NetworkFactory::CookiePolicy;

  QTest::newRow("application-allow") << CookiePolicy::UseApplicationSetting << false << CookiePolicy::AllowCookies;
  QTest::newRow("application-ignore") << CookiePolicy::UseApplicationSetting << true << CookiePolicy::IgnoreCookies;
  QTest::newRow("force-ignore-global-allow") << CookiePolicy::IgnoreCookies << false << CookiePolicy::IgnoreCookies;
  QTest::newRow("force-ignore-global-ignore") << CookiePolicy::IgnoreCookies << true << CookiePolicy::IgnoreCookies;
  QTest::newRow("force-allow-global-allow") << CookiePolicy::AllowCookies << false << CookiePolicy::AllowCookies;
  QTest::newRow("force-allow-global-ignore") << CookiePolicy::AllowCookies << true << CookiePolicy::AllowCookies;
  QTest::newRow("invalid-global-allow") << static_cast<CookiePolicy>(99) << false << CookiePolicy::AllowCookies;
  QTest::newRow("invalid-global-ignore") << static_cast<CookiePolicy>(99) << true << CookiePolicy::IgnoreCookies;
}

void TestWebFactory::resolvesCookiePolicy() {
  QFETCH(NetworkFactory::CookiePolicy, requested);
  QFETCH(bool, ignore_all_cookies);
  QFETCH(NetworkFactory::CookiePolicy, expected);

  QCOMPARE(NetworkFactory::resolveCookiePolicy(requested, ignore_all_cookies), expected);
}

QTEST_APPLESS_MAIN(TestWebFactory)

#include "test_webfactory.moc"
