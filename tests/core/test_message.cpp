// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/message.h"

#include <QTest>

class TestMessage : public QObject {
    Q_OBJECT

  private slots:
    void serializesEnclosures();
    void rejectsInvalidEnclosureData();
    void comparesStableIdentifiers();
};

void TestMessage::serializesEnclosures() {
  QList<QSharedPointer<MessageEnclosure>> source;

  source.append(QSharedPointer<MessageEnclosure>::create(QStringLiteral("https://example.com/audio.mp3"),
                                                         QStringLiteral("audio/mpeg")));
  source.append(QSharedPointer<MessageEnclosure>::create(QStringLiteral("https://example.com/image.png"),
                                                         QStringLiteral("image/png")));

  const QString serialized = Enclosures::encodeEnclosuresToString(source);
  const QList<QSharedPointer<MessageEnclosure>> restored = Enclosures::decodeEnclosuresFromString(serialized);

  QCOMPARE(restored.size(), source.size());
  QCOMPARE(restored.at(0)->url(), source.at(0)->url());
  QCOMPARE(restored.at(0)->mimeType(), source.at(0)->mimeType());
  QCOMPARE(restored.at(1)->url(), source.at(1)->url());
  QCOMPARE(restored.at(1)->mimeType(), source.at(1)->mimeType());
}

void TestMessage::rejectsInvalidEnclosureData() {
  QVERIFY(Enclosures::decodeEnclosuresFromString(QStringLiteral("not JSON")).isEmpty());
}

void TestMessage::comparesStableIdentifiers() {
  Message first;
  Message second;

  first.m_accountId = second.m_accountId = 7;
  first.m_id = second.m_id = 15;
  QVERIFY(first == second);

  second.m_accountId = 8;
  QVERIFY(first != second);

  first.m_accountId = second.m_accountId = 7;
  first.m_id = second.m_id = 0;
  first.m_customId = second.m_customId = QStringLiteral("remote-id");
  QVERIFY(first == second);
}

QTEST_APPLESS_MAIN(TestMessage)

#include "test_message.moc"
