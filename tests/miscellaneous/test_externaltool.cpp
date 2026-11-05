// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QVariant>

class TestExternalTool : public QObject {
    Q_OBJECT

  private slots:
    void roundTripsCustomIcon();
    void copiesKeepCustomIcon();
    void loadsLegacyConfiguration();
    void invalidIconUsesExecutableIcon();
    void iconSurvivesSourceFileRemoval();
    void persistsAndResetsIcon();
};

namespace {
  QByteArray customIconData() {
    QPixmap pixmap(24, 24);
    pixmap.fill(QColor(35, 90, 170));
    return IconFactory::toByteArray(QIcon(pixmap));
  }

  QImage automaticIcon(const ExternalTool& tool) {
    return QFileIconProvider().icon(QFileInfo(tool.executable())).pixmap(32, 32).toImage();
  }
} // namespace

void TestExternalTool::roundTripsCustomIcon() {
  const QByteArray icon_data = customIconData();
  ExternalTool source(QStringLiteral("Player"),
                      QStringLiteral("player"),
                      QStringLiteral("--url %1"),
                      {QStringLiteral("example.com")},
                      icon_data);
  const QByteArray serialized = source.toString();
  const ExternalTool restored = ExternalTool::fromString(serialized);

  QCOMPARE(QJsonDocument::fromJson(serialized).object().value(QStringLiteral("icon")).toString().toLatin1(), icon_data);
  QCOMPARE(restored.name(), source.name());
  QCOMPARE(restored.executable(), source.executable());
  QCOMPARE(restored.parameters(), source.parameters());
  QCOMPARE(restored.domains(), source.domains());
  QCOMPARE(restored.iconData(), icon_data);
  QCOMPARE(restored.icon().pixmap(24, 24).toImage(), IconFactory::fromByteArray(icon_data).pixmap(24, 24).toImage());
}

void TestExternalTool::copiesKeepCustomIcon() {
  const QByteArray icon_data = customIconData();
  const ExternalTool source(QStringLiteral("Player"),
                            QStringLiteral("player"),
                            {},
                            {QStringLiteral("example.com")},
                            icon_data);
  const ExternalTool copied(source);
  ExternalTool assigned;
  assigned = source;
  const ExternalTool variant_copy = QVariant::fromValue(source).value<ExternalTool>();
  const auto matched = ExternalTool::toolForDomain({source}, QStringLiteral("www.example.com"));

  QCOMPARE(copied.iconData(), icon_data);
  QCOMPARE(assigned.iconData(), icon_data);
  QCOMPARE(variant_copy.iconData(), icon_data);
  QVERIFY(matched.has_value());
  QCOMPARE(matched->iconData(), icon_data);
}

void TestExternalTool::loadsLegacyConfiguration() {
  const ExternalTool tool =
    ExternalTool::fromString(R"({"name":"Player","exe":"player","params":"%1","domain":"example.com"})");

  QCOMPARE(tool.name(), QStringLiteral("Player"));
  QCOMPARE(tool.parameters(), QStringLiteral("%1"));
  QCOMPARE(tool.domains(), QStringList{QStringLiteral("example.com")});
  QVERIFY(tool.iconData().isEmpty());
  QCOMPARE(tool.icon().pixmap(32, 32).toImage(), automaticIcon(tool));
}

void TestExternalTool::invalidIconUsesExecutableIcon() {
  const ExternalTool tool(QStringLiteral("Player"),
                          QCoreApplication::applicationFilePath(),
                          {},
                          {},
                          QByteArray("invalid icon data").toBase64());

  QCOMPARE(tool.icon().pixmap(32, 32).toImage(), automaticIcon(tool));
}

void TestExternalTool::iconSurvivesSourceFileRemoval() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("custom.png"));
  QPixmap source(24, 24);
  source.fill(QColor(150, 65, 25));
  QVERIFY(source.save(path));
  ExternalTool tool(QStringLiteral("Player"),
                    QStringLiteral("player"),
                    {},
                    {},
                    IconFactory::toByteArray(QIcon(QPixmap(path))));
  const QByteArray serialized = tool.toString();
  QVERIFY(QFile::remove(path));

  const ExternalTool restored = ExternalTool::fromString(serialized);
  QCOMPARE(restored.icon().pixmap(24, 24).toImage(), source.toImage());
}

void TestExternalTool::persistsAndResetsIcon() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("config.ini"));
  const QByteArray icon_data = customIconData();
  QVector<ExternalTool> tools{ExternalTool(QStringLiteral("Player"), QStringLiteral("player"), {}, {}, icon_data)};

  {
    Settings settings(path, QSettings::IniFormat, SettingsProperties::SettingsType::Custom);
    ExternalTool::setToolsToSettings(tools, &settings);
    QCOMPARE(settings.checkSettings(), QSettings::Status::NoError);
  }

  Settings settings(path, QSettings::IniFormat, SettingsProperties::SettingsType::Custom);
  const auto restored = ExternalTool::toolsFromSettings(&settings);
  QCOMPARE(restored.size(), 1);
  QCOMPARE(restored.first().iconData(), icon_data);

  tools[0] = ExternalTool(restored.first().name(),
                          restored.first().executable(),
                          restored.first().parameters(),
                          restored.first().domains());
  ExternalTool::setToolsToSettings(tools, &settings);
  QCOMPARE(settings.checkSettings(), QSettings::Status::NoError);
  const auto reset = ExternalTool::toolsFromSettings(&settings);
  QCOMPARE(reset.size(), 1);
  QVERIFY(reset.first().iconData().isEmpty());
  QCOMPARE(reset.first().icon().pixmap(32, 32).toImage(), automaticIcon(reset.first()));
}

QTEST_MAIN(TestExternalTool)

#include "test_externaltool.moc"
