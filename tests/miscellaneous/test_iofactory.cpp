// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/ioexception.h"
#include "miscellaneous/iofactory.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class TestIOFactory : public QObject {
    Q_OBJECT

  private slots:
    void writesReadsAndCopiesFiles();
    void createsUniqueFilename();
    void filtersInvalidFilenameCharacters();
    void reportsMissingFile();
    void removesFolderRecursively();
};

void TestIOFactory::writesReadsAndCopiesFiles() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  QVERIFY(IOFactory::isFolderWritable(directory.path()));

  const QString source_path = directory.filePath(QStringLiteral("source.bin"));
  const QString destination_path = directory.filePath(QStringLiteral("destination.bin"));
  const QByteArray original_data("RSS\0Guard", 9);
  const QByteArray replacement_data("replacement");

  IOFactory::writeFile(source_path, original_data);
  QCOMPARE(IOFactory::readFile(source_path), original_data);

  IOFactory::writeFile(destination_path, replacement_data);
  QVERIFY(IOFactory::copyFile(source_path, destination_path));
  QCOMPARE(IOFactory::readFile(destination_path), original_data);
}

void TestIOFactory::createsUniqueFilename() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString original_path = directory.filePath(QStringLiteral("article.html"));
  const QString first_copy_path = directory.filePath(QStringLiteral("article(1).html"));

  IOFactory::writeFile(original_path, {});
  IOFactory::writeFile(first_copy_path, {});

  QCOMPARE(IOFactory::ensureUniqueFilename(original_path),
           directory.filePath(QStringLiteral("article(2).html")));
  QCOMPARE(IOFactory::ensureUniqueFilename(directory.filePath(QStringLiteral("unused.html"))),
           directory.filePath(QStringLiteral("unused.html")));
}

void TestIOFactory::filtersInvalidFilenameCharacters() {
  QCOMPARE(IOFactory::filterBadCharsFromFilename(QStringLiteral("a/b\\c:d*e?f\"g<h>i|j")),
           QStringLiteral("a-bcdefghij"));
}

void TestIOFactory::reportsMissingFile() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  bool exception_thrown = false;

  try {
    IOFactory::readFile(directory.filePath(QStringLiteral("missing")));
  }
  catch (const IOException&) {
    exception_thrown = true;
  }

  QVERIFY(exception_thrown);
}

void TestIOFactory::removesFolderRecursively() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString nested_folder = directory.filePath(QStringLiteral("one/two"));
  QVERIFY(QDir().mkpath(nested_folder));
  IOFactory::writeFile(QDir(nested_folder).filePath(QStringLiteral("file")), QByteArray("data"));

  IOFactory::removeFolder(directory.filePath(QStringLiteral("one")));
  QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("one"))));
}

QTEST_APPLESS_MAIN(TestIOFactory)

#include "test_iofactory.moc"
