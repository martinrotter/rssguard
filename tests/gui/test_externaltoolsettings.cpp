// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsbrowsermail.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"

#include <QDir>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

namespace {
  QByteArray seedTools(Settings& settings) {
    QPixmap pixmap(24, 24);
    pixmap.fill(QColor(30, 100, 180));
    const QByteArray icon_data = IconFactory::toByteArray(QIcon(pixmap));
    QVector<ExternalTool> tools{
      ExternalTool(QSL("Automatic"), QCoreApplication::applicationFilePath(), {}, {}),
      ExternalTool(QSL("Custom"), QCoreApplication::applicationFilePath(), {}, {}, icon_data)};
    ExternalTool::setToolsToSettings(tools, &settings);
    return icon_data;
  }

  QImage rowIcon(const QTreeWidgetItem* item) {
    return item->icon(0).pixmap(32, 32).toImage();
  }

  QImage expectedIcon(const QTreeWidgetItem* item, const QByteArray& icon_data = {}) {
    return ExternalTool({}, item->text(1), {}, {}, icon_data).icon().pixmap(32, 32).toImage();
  }
} // namespace

class TestExternalToolSettings : public QObject {
    Q_OBJECT

  private slots:
    void loadingAndSelectionAreNotDirty();
    void executableEditsPreserveCustomIcon();
    void resetHonorsCancelAndApply();
};

void TestExternalToolSettings::loadingAndSelectionAreNotDirty() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  Settings settings(directory.filePath(QSL("config.ini")),
                    QSettings::IniFormat,
                    SettingsProperties::SettingsType::Custom);
  const QByteArray custom = seedTools(settings);
  SettingsBrowserMail panel(&settings);
  panel.loadUi();
  panel.loadSettings();
  auto* list = panel.findChild<QTreeWidget*>(QSL("m_listTools"));
  auto* button = panel.findChild<QPushButton*>(QSL("m_btnToolIcon"));
  QVERIFY(list != nullptr);
  QVERIFY(button != nullptr);
  QCOMPARE(list->topLevelItemCount(), 2);
  QCOMPARE(rowIcon(list->topLevelItem(0)), expectedIcon(list->topLevelItem(0)));
  QCOMPARE(rowIcon(list->topLevelItem(1)), expectedIcon(list->topLevelItem(1), custom));
  QVERIFY(!panel.isDirty());

  QSignalSpy changes(&panel, &SettingsPanel::settingsChanged);
  list->setCurrentItem(list->topLevelItem(0));
  QVERIFY(button->isEnabled());
  list->setCurrentItem(list->topLevelItem(1));
  list->setCurrentItem(nullptr);
  QVERIFY(!button->isEnabled());
  QVERIFY(!panel.isDirty());
  QCOMPARE(changes.count(), 0);
}

void TestExternalToolSettings::executableEditsPreserveCustomIcon() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  Settings settings(directory.filePath(QSL("config.ini")),
                    QSettings::IniFormat,
                    SettingsProperties::SettingsType::Custom);
  const QByteArray custom = seedTools(settings);
  SettingsBrowserMail panel(&settings);
  panel.loadUi();
  panel.loadSettings();
  auto* list = panel.findChild<QTreeWidget*>(QSL("m_listTools"));
  QVERIFY(list != nullptr);
  auto* automatic = list->topLevelItem(0);
  auto* overridden = list->topLevelItem(1);
  const QImage original_custom = rowIcon(overridden);
  automatic->setText(1, directory.path());
  overridden->setText(1, directory.path());
  QVERIFY(panel.isDirty());
  QVERIFY(automatic->data(0, Qt::UserRole).toByteArray().isEmpty());
  QCOMPARE(rowIcon(automatic), expectedIcon(automatic));
  QCOMPARE(overridden->data(0, Qt::UserRole).toByteArray(), custom);
  QCOMPARE(rowIcon(overridden), original_custom);
  panel.saveSettings();
  QVERIFY(!panel.isDirty());

  SettingsBrowserMail reloaded(&settings);
  reloaded.loadUi();
  reloaded.loadSettings();
  auto* reloaded_list = reloaded.findChild<QTreeWidget*>(QSL("m_listTools"));
  QVERIFY(reloaded_list != nullptr);
  QCOMPARE(reloaded_list->topLevelItem(1)->data(0, Qt::UserRole).toByteArray(), custom);
  QCOMPARE(rowIcon(reloaded_list->topLevelItem(1)), original_custom);
}

void TestExternalToolSettings::resetHonorsCancelAndApply() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  Settings settings(directory.filePath(QSL("config.ini")),
                    QSettings::IniFormat,
                    SettingsProperties::SettingsType::Custom);
  const QByteArray custom = seedTools(settings);
  for (bool apply : {false, true}) {
    SettingsBrowserMail panel(&settings);
    panel.loadUi();
    panel.loadSettings();
    auto* list = panel.findChild<QTreeWidget*>(QSL("m_listTools"));
    QVERIFY(list != nullptr);
    auto* item = list->topLevelItem(1);
    list->setCurrentItem(item);
    QVERIFY(QMetaObject::invokeMethod(&panel, "resetSelectedExternalToolIcon", Qt::DirectConnection));
    QVERIFY(panel.isDirty());
    QVERIFY(item->data(0, Qt::UserRole).toByteArray().isEmpty());
    QCOMPARE(rowIcon(item), expectedIcon(item));
    QCOMPARE(ExternalTool::toolsFromSettings(&settings).at(1).iconData(), custom);
    if (apply) {
      panel.saveSettings();
      QVERIFY(!panel.isDirty());
    }
  }
  SettingsBrowserMail reloaded(&settings);
  reloaded.loadUi();
  reloaded.loadSettings();
  auto* list = reloaded.findChild<QTreeWidget*>(QSL("m_listTools"));
  QVERIFY(list != nullptr);
  QVERIFY(list->topLevelItem(1)->data(0, Qt::UserRole).toByteArray().isEmpty());
  QCOMPARE(rowIcon(list->topLevelItem(1)), expectedIcon(list->topLevelItem(1)));
  QVERIFY(!reloaded.isDirty());
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  QDir().mkpath(directory.path() + QDir::separator() + QSL(APP_CFG_PATH));
  QSettings settings(directory.path() + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE),
                     QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.sync();
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        directory.path(),
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-external-tool-settings-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  TestExternalToolSettings tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_externaltoolsettings.moc"
