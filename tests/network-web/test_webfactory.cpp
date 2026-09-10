// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webfactory.h"

#include <QTest>

class TestWebFactory : public QObject {
    Q_OBJECT

  private slots:
    void unescapesAllHtmlEntities();
    void unescapesOnlyNumericEntitiesAboveBoundary();
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

  QCOMPARE(WebFactory::unescapeHtml(
             QStringLiteral("&amp; &#127; &#128; &#353; &#x161; &#X2013; &#60; &#xD800; &#x110000;"), char32_t{0x80}),
           expected);
}

QTEST_APPLESS_MAIN(TestWebFactory)

#include "test_webfactory.moc"
