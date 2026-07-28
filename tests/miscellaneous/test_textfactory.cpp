// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textfactory.h"

#include <QLocale>
#include <QTest>

#include <utility>

class TestTextFactory : public QObject {
    Q_OBJECT

  private slots:
    void extractsUsernameFromEmail();
    void recognizesHtml();
    void createsUniqueName();
    void parsesDateTime_data();
    void parsesDateTime();
    void benchmarksDateTimeParsing();
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

void TestTextFactory::benchmarksDateTimeParsing() {
  constexpr int date_count = 10000;
  const QDateTime base_date(QDate(2020, 1, 1), QTime(0, 0), Qt::TimeSpec::UTC);
  const QLocale locale(QLocale::Language::C);
  QStringList dates;

  dates.reserve(date_count);

  for (int i = 0; i < date_count; ++i) {
    const QDateTime date = base_date.addSecs(i * 3600);

    switch (i % 5) {
      case 0:
        dates.append(date.toString(Qt::DateFormat::ISODate));
        break;
      case 1:
        dates.append(date.toString(Qt::DateFormat::RFC2822Date));
        break;
      case 2:
        dates.append(date.toString(QSL("yyyy-MM-dd HH:mm:ss")));
        break;
      case 3:
        dates.append(date.toString(QSL("yyyy-MM-dd HH:mm:ss")) + QSL(" +0000"));
        break;
      case 4:
        dates.append(locale.toString(date, QSL("d MMMM yyyy HH:mm:ss")) + QSL(" +0000"));
        break;
    }
  }

  int valid_dates = 0;

  QBENCHMARK {
    valid_dates = 0;

    for (const QString& date : std::as_const(dates)) {
      valid_dates += int(TextFactory::parseDateTime(date).isValid());
    }
  }

  QCOMPARE(valid_dates, date_count);
}

QTEST_APPLESS_MAIN(TestTextFactory)

#include "test_textfactory.moc"
