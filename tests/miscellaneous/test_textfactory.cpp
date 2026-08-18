// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textfactory.h"

#include <QTest>

class TestTextFactory : public QObject {
    Q_OBJECT

  private slots:
    void extractsUsernameFromEmail();
    void recognizesHtml();
    void createsUniqueName();
    void parsesDateTime_data();
    void parsesDateTime();
};

void TestTextFactory::extractsUsernameFromEmail() {
  QCOMPARE(TextFactory::extractUsernameFromEmail(QSL("reader@example.com")), QSL("reader"));
  QCOMPARE(TextFactory::extractUsernameFromEmail(QSL("reader")), QSL("reader"));
}

void TestTextFactory::recognizesHtml() {
  QVERIFY(TextFactory::couldBeHtml(QSL("<article>Contents</article>")));
  QVERIFY(!TextFactory::couldBeHtml(QSL("Plain article contents")));
}

void TestTextFactory::createsUniqueName() {
  const QStringList existing_names{QSL("Feed"), QSL("Feed (1)"), QSL("Feed (2)")};

  QCOMPARE(TextFactory::ensureUniqueName(QSL("New feed"), existing_names, QSL(" (%1)")), QSL("New feed"));
  QCOMPARE(TextFactory::ensureUniqueName(QSL("Feed"), existing_names, QSL(" (%1)")), QSL("Feed (3)"));
}

void TestTextFactory::parsesDateTime_data() {
  QTest::addColumn<QString>("input");
  QTest::addColumn<bool>("valid");

  QTest::newRow("plain-seconds") << QSL("2026-06-18 14:37:00") << true;
  QTest::newRow("plain-minutes") << QSL("2026-07-27 11:26") << true;
  QTest::newRow("plain-numeric-offset") << QSL("2026-07-28 11:37:18 +0800") << true;
  QTest::newRow("rfc-numeric-offset") << QSL("Sun, 19 Jul 2026 00:00:00 -0800") << true;
  QTest::newRow("rfc-single-digit-fields") << QSL("Thu, 02 Sep 2021 20:0:0 Z") << true;
  QTest::newRow("literal-at") << QSL("Wed, 13 Feb 2019 at 16:00:00") << true;
  QTest::newRow("month-comma") << QSL("24 Jul, 2026 +0530") << true;
  QTest::newRow("full-month") << QSL("15 January 2025 00:00:00 +0000") << true;
  QTest::newRow("invalid-hour") << QSL("Thu, 12 May 2022 24:57:16 +0000") << false;
  QTest::newRow("invalid-literal") << QSL("Invalid Date") << false;
}

void TestTextFactory::parsesDateTime() {
  QFETCH(QString, input);
  QFETCH(bool, valid);

  QCOMPARE(TextFactory::parseDateTime(input).isValid(), valid);
}

QTEST_APPLESS_MAIN(TestTextFactory)

#include "test_textfactory.moc"
