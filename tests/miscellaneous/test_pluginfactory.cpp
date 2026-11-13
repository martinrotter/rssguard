// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/pluginfactory.h"
#include "services/abstract/serviceentrypoint.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class TestPluginFactory : public QObject {
    Q_OBJECT

  private slots:
    void rejectsBeforeConstruction_data() {
      QTest::addColumn<QString>("path");
      QTest::newRow("missing") << QStringLiteral(TEST_PLUGIN_MISSING);
      QTest::newRow("mismatched") << QStringLiteral(TEST_PLUGIN_MISMATCHED);
    }

    void rejectsBeforeConstruction() {
      QFETCH(QString, path);
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString marker = directory.filePath(QStringLiteral("constructor"));
      qputenv("RSSGUARD_PLUGIN_CONSTRUCTOR_MARKER", marker.toUtf8());
      QString error;
      QVERIFY(PluginFactory::loadPlugin(path, &error) == nullptr);
      QVERIFY(!error.isEmpty());
      QVERIFY(!QFileInfo::exists(marker));
      qunsetenv("RSSGUARD_PLUGIN_CONSTRUCTOR_MARKER");
    }

    void acceptsCurrentApi() {
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString marker = directory.filePath(QStringLiteral("constructor"));
      qputenv("RSSGUARD_PLUGIN_CONSTRUCTOR_MARKER", marker.toUtf8());
      QString error;
      auto* plugin = PluginFactory::loadPlugin(QStringLiteral(TEST_PLUGIN_CURRENT), &error);
      QVERIFY2(plugin != nullptr, qPrintable(error));
      QVERIFY(error.isEmpty());
      QVERIFY(QFileInfo::exists(marker));
      QCOMPARE(plugin->code(), QStringLiteral("fixture"));
      qunsetenv("RSSGUARD_PLUGIN_CONSTRUCTOR_MARKER");
    }
};

QTEST_MAIN(TestPluginFactory)
#include "test_pluginfactory.moc"
