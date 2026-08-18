// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textfactory.h"

#include <QLocale>
#include <QTest>

#include <utility>

class BenchmarkTextFactory : public QObject {
    Q_OBJECT

  private slots:
    void benchmarksDateTimeParsing();
};

void BenchmarkTextFactory::benchmarksDateTimeParsing() {
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

QTEST_APPLESS_MAIN(BenchmarkTextFactory)

#include "benchmark_textfactory.moc"
