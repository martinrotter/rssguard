// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textfactory.h"

#include <QTest>

class TestTextFactory : public QObject {
    Q_OBJECT

  private slots:
    void extractsUsernameFromEmail();
    void recognizesHtml();
    void createsUniqueName();
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

QTEST_APPLESS_MAIN(TestTextFactory)

#include "test_textfactory.moc"
