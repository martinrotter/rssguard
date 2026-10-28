// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/message.h"

#include <QDataStream>
#include <QTest>

class TestMessage : public QObject {
    Q_OBJECT

  private slots:
    void serializesEnclosures();
    void rejectsInvalidEnclosureData();
    void comparesStableIdentifiers();
    void overwriteIsTransientButCopied();
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

void TestMessage::overwriteIsTransientButCopied() {
  Message source;
  QVERIFY(!source.m_overwrite);
  source.m_overwrite = true;
  source.m_customId = QStringLiteral("overwrite-test");
  source.m_isRead = true;
  QVERIFY(Message(source).m_overwrite);
  Message assigned;
  assigned = source;
  QVERIFY(assigned.m_overwrite);

  QByteArray bytes;
  QDataStream output(&bytes, QIODevice::WriteOnly);
  output << source;
  Message without_flag = source;
  without_flag.m_overwrite = false;
  QByteArray original_format;
  QDataStream expected(&original_format, QIODevice::WriteOnly);
  expected << without_flag;
  QCOMPARE(bytes, original_format);

  Message restored;
  restored.m_overwrite = true;
  QDataStream input(bytes);
  input >> restored;
  QCOMPARE(input.status(), QDataStream::Ok);
  QVERIFY(!restored.m_overwrite);
  QVERIFY(restored.m_isRead);
  QCOMPARE(restored.m_customId, source.m_customId);
}

QTEST_APPLESS_MAIN(TestMessage)

#include "test_message.moc"
