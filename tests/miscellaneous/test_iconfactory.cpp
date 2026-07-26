// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/iconfactory.h"

#include <QTest>

class TestIconFactory : public QObject {
    Q_OBJECT

  private slots:
    void choosesReadableTextColor();
    void recolorsOnlyPlaceholderPixels();
    void serializesIcon();
    void serializesPixmap();
};

void TestIconFactory::choosesReadableTextColor() {
  QCOMPARE(IconFactory::readableTextColor(Qt::GlobalColor::white), QColor(Qt::GlobalColor::black));
  QCOMPARE(IconFactory::readableTextColor(Qt::GlobalColor::black), QColor(Qt::GlobalColor::white));
}

void TestIconFactory::recolorsOnlyPlaceholderPixels() {
  QImage source(3, 1, QImage::Format_ARGB32);

  source.setPixelColor(0, 0, QColor(255, 0, 0, 73));
  source.setPixelColor(1, 0, QColor(254, 0, 0, 91));
  source.setPixelColor(2, 0, QColor(0, 0, 255, 109));

  const QColor replacement(23, 101, 177);
  const QImage result = IconFactory::recolorImage(source, replacement);

  QCOMPARE(result.pixelColor(0, 0), QColor(23, 101, 177, 73));
  QCOMPARE(result.pixelColor(1, 0), QColor(254, 0, 0, 91));
  QCOMPARE(result.pixelColor(2, 0), QColor(0, 0, 255, 109));
}

void TestIconFactory::serializesIcon() {
  const QIcon source = IconFactory::fromColor(QColor(180, 30, 90), QLatin1Char('R'));
  const QByteArray serialized = IconFactory::toByteArray(source);
  const QIcon restored = IconFactory::fromByteArray(serialized);

  QVERIFY(!source.isNull());
  QVERIFY(!serialized.isEmpty());
  QVERIFY(!restored.isNull());
  QCOMPARE(IconFactory::iconGuid(restored), IconFactory::iconGuid(source));
}

void TestIconFactory::serializesPixmap() {
  QPixmap source(4, 3);
  source.fill(QColor(12, 34, 56));

  const QByteArray serialized = IconFactory::toByteArray(source, QStringLiteral("PNG"));
  const QPixmap restored = IconFactory::fromByteArray(serialized, QStringLiteral("PNG"));

  QVERIFY(!serialized.isEmpty());
  QCOMPARE(restored.size(), source.size());
  QCOMPARE(restored.toImage().pixelColor(0, 0), QColor(12, 34, 56));
}

QTEST_MAIN(TestIconFactory)

#include "test_iconfactory.moc"
