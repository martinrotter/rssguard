// For license of this file, see <project-root-folder>/LICENSE.md.

#include "definitions/globals.h"

#include <QTest>

class TestGlobals : public QObject {
    Q_OBJECT

  private:
    enum class TestFlag {
      First = 1,
      Second = 2,
      Third = 4
    };

  private slots:
    void recognizesCombinedFlags();
    void rejectsMissingFlags();
};

void TestGlobals::recognizesCombinedFlags() {
  const int flags = int(TestFlag::First) | int(TestFlag::Third);

  QVERIFY(Globals::hasFlag(flags, TestFlag::First));
  QVERIFY(Globals::hasFlag(flags, TestFlag::Third));
}

void TestGlobals::rejectsMissingFlags() {
  const int flags = int(TestFlag::Second);

  QVERIFY(!Globals::hasFlag(flags, TestFlag::First));
  QVERIFY(!Globals::hasFlag(flags, TestFlag::Third));
}

QTEST_APPLESS_MAIN(TestGlobals)

#include "test_globals.moc"
