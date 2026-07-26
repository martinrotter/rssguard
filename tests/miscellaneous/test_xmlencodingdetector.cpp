// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/xmlencodingdetector.h"

#include <QTest>

class TestXmlEncodingDetector : public QObject {
    Q_OBJECT

  private slots:
    void defaultsToUtf8();
    void readsEncodingFromDeclaration();
    void detectsByteOrderMark();
};

void TestXmlEncodingDetector::defaultsToUtf8() {
  QCOMPARE(XmlEncodingDetector::detectXmlEncoding({}), QStringLiteral("UTF-8"));
  QCOMPARE(XmlEncodingDetector::detectXmlEncoding("<feed/>"), QStringLiteral("UTF-8"));
}

void TestXmlEncodingDetector::readsEncodingFromDeclaration() {
  const QByteArray xml = "<?xml version=\"1.0\" encoding=\"iso-8859-2\"?><feed/>";

  QCOMPARE(XmlEncodingDetector::detectXmlEncoding(xml), QStringLiteral("ISO-8859-2"));
}

void TestXmlEncodingDetector::detectsByteOrderMark() {
  QCOMPARE(XmlEncodingDetector::detectXmlEncoding(QByteArray::fromHex("fffe")), QStringLiteral("UTF-16LE"));
  QCOMPARE(XmlEncodingDetector::detectXmlEncoding(QByteArray::fromHex("fffe00003c000000")), QStringLiteral("UTF-32LE"));
}

QTEST_APPLESS_MAIN(TestXmlEncodingDetector)

#include "test_xmlencodingdetector.moc"
