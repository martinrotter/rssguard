// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

class TestSettings : public QObject {
    Q_OBJECT

  private slots:
    void storesAndLoadsValues();
    void listsAndRemovesValues();
    void usesProvidedDefaults();
};

void TestSettings::storesAndLoadsValues() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString settings_path = directory.filePath(QStringLiteral("config.ini"));
  Settings settings(settings_path, QSettings::IniFormat, SettingsProperties::SettingsType::Custom);

  QCOMPARE(settings.type(), SettingsProperties::SettingsType::Custom);
  QCOMPARE(QDir::cleanPath(settings.pathName()), QDir::cleanPath(directory.path()));
  QVERIFY(!settings.contains(QStringLiteral("test"), QStringLiteral("number")));

  settings.setValue(QStringLiteral("test"), QStringLiteral("number"), 42);
  QCOMPARE(settings.value(QStringLiteral("test"), QStringLiteral("number")).toInt(), 42);
  QCOMPARE(settings.checkSettings(), QSettings::Status::NoError);

  QSettings persisted(settings_path, QSettings::IniFormat);
  QCOMPARE(persisted.value(QStringLiteral("test/number")).toInt(), 42);
}

void TestSettings::listsAndRemovesValues() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  Settings settings(directory.filePath(QStringLiteral("config.ini")),
                    QSettings::IniFormat,
                    SettingsProperties::SettingsType::Custom);

  settings.setValue(QStringLiteral("group"), QStringLiteral("first"), 1);
  settings.setValue(QStringLiteral("group"), QStringLiteral("second"), 2);

  QCOMPARE(settings.allKeys(QStringLiteral("group")),
           QStringList({QStringLiteral("first"), QStringLiteral("second")}));

  settings.remove(QStringLiteral("group"), QStringLiteral("first"));
  QVERIFY(!settings.contains(QStringLiteral("group"), QStringLiteral("first")));
  QVERIFY(settings.contains(QStringLiteral("group"), QStringLiteral("second")));

  settings.remove(QStringLiteral("group"));
  QVERIFY(settings.allKeys(QStringLiteral("group")).isEmpty());
}

void TestSettings::usesProvidedDefaults() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  Settings settings(directory.filePath(QStringLiteral("config.ini")),
                    QSettings::IniFormat,
                    SettingsProperties::SettingsType::Custom);

  QCOMPARE(settings.value(GROUP(Network), SETTING(Network::IgnoreAllCookies)).toBool(),
           DEFAULT_VALUE(Network::IgnoreAllCookies));
  QCOMPARE(settings.value(GROUP(Messages), SETTING(Messages::AlwaysDisplayItemPreview)).toBool(),
           DEFAULT_VALUE(Messages::AlwaysDisplayItemPreview));
}

QTEST_MAIN(TestSettings)

#include "test_settings.moc"
