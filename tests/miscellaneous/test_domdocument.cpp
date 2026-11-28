// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/domdocument.h"

#include <QTest>

class TestDomDocument : public QObject {
    Q_OBJECT

  private slots:
    void parsesXml();
    void normalizesCommonInvalidContents();
    void rejectsMalformedXml();
};

void TestDomDocument::parsesXml() {
  DomDocument document;

  QVERIFY(document.setContent(QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?><feed><title>News</title></feed>"),
                              true));
  QCOMPARE(document.documentElement().tagName(), QStringLiteral("feed"));
  QCOMPARE(document.documentElement().firstChildElement(QStringLiteral("title")).text(), QStringLiteral("News"));
}

void TestDomDocument::normalizesCommonInvalidContents() {
  DomDocument document;

  QVERIFY(document.setContent(QStringLiteral("<root>Rock & Roll &shy;&rsquo;</root>"), false));
  QCOMPARE(document.documentElement().text(), QStringLiteral("Rock & Roll '"));

  QString dirty = QStringLiteral("<root>A & & B");

  dirty += QChar(0);
  dirty += QChar(0x08);
  dirty += QChar(0x86);
  dirty += QStringLiteral("</root>");
  QVERIFY(document.setContent(dirty, false));
  QCOMPARE(document.documentElement().text(), QStringLiteral("A & & B"));
}

void TestDomDocument::rejectsMalformedXml() {
  DomDocument document;
  QString error;
  int error_line = 0;
  int error_column = 0;

  QVERIFY(!document.setContent(QStringLiteral("<root><child></root>"),
                               false,
                               &error,
                               &error_line,
                               &error_column));
  QVERIFY(!error.isEmpty());
  QVERIFY(error_line > 0);
  QVERIFY(error_column > 0);
}

QTEST_APPLESS_MAIN(TestDomDocument)

#include "test_domdocument.moc"
