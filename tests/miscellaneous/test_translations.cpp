// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/windowstaskbar.h"

#include <librssguard-greader/src/gui/formgreaderfeeddetails.h>
#include <librssguard-nextcloud/src/gui/formnextcloudfeeddetails.h>
#include <librssguard-ttrss/src/gui/formttrssfeeddetails.h>

#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>
#include <QXmlStreamReader>

namespace {
  bool isApplicationSource(const QString& source_root, const QString& path) {
    const QString relative = QDir(source_root + QSL("/src")).relativeFilePath(path);
    const QStringList components = relative.split(QL1C('/'));
    return relative.startsWith(QSL("librssguard")) && !components.contains(QSL("build")) &&
           !components.contains(QSL("CMakeFiles")) &&
           (!components.contains(QSL("3rd-party")) ||
            relative.startsWith(QSL("librssguard-gmail/src/3rd-party/richtexteditor/")));
  }

  class ContextTranslator : public QTranslator {
    public:
      bool isEmpty() const override {
        return false;
      }

      QString translate(const char* context, const char* source, const char*, int) const override {
        return QString::fromLatin1(context) + QSL("|") + QString::fromUtf8(source);
      }
  };

  QSet<QString> catalogKeys(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      return {};
    }
    QXmlStreamReader xml(&file);
    QSet<QString> keys;
    QString context;
    while (!xml.atEnd()) {
      xml.readNext();
      if (!xml.isStartElement() || xml.name() == QL1S("TS") || xml.name() == QL1S("context")) {
        continue;
      }
      if (xml.name() == QL1S("name")) {
        context = xml.readElementText();
      }
      else if (xml.name() == QL1S("message")) {
        const QString numerus = xml.attributes().value(QSL("numerus")).toString();
        QString source, comment;
        while (xml.readNextStartElement()) {
          if (xml.name() == QL1S("source")) {
            source = xml.readElementText();
          }
          else if (xml.name() == QL1S("comment")) {
            comment = xml.readElementText();
          }
          else {
            xml.skipCurrentElement();
          }
        }
        keys.insert(QStringList{context, source, comment, numerus}.join(QChar(0x1f)));
      }
      else {
        xml.skipCurrentElement();
      }
    }
    return xml.hasError() ? QSet<QString>{} : keys;
  }
} // namespace

class TestTranslations : public QObject {
    Q_OBJECT

  private slots:
    void runtimeContexts();
    void generatorRowContexts();
#if QT_VERSION_MAJOR >= 6
    void extractionCoverage();
    void failedUpdatesPreserveCatalog_data();
    void failedUpdatesPreserveCatalog();
    void checkoutPaths_data();
    void checkoutPaths();
    void catalogDiscoveryIgnoresStaging();
#endif
};

void TestTranslations::runtimeContexts() {
  ContextTranslator translator;
  QVERIFY(QCoreApplication::installTranslator(&translator));
  const QString greader = FormGreaderFeedDetails::tr("Feed added");
  const QString nextcloud = FormNextcloudFeedDetails::tr("Feed added");
  const QString ttrss = FormTtRssFeedDetails::tr("Feed added");
#if defined(Q_OS_WIN)
  const QString taskbar = WindowsTaskbar::tr("Resume automatic feed fetching");
#endif
  QCoreApplication::removeTranslator(&translator);
  QCOMPARE(greader, QSL("FormGreaderFeedDetails|Feed added"));
  QCOMPARE(nextcloud, QSL("FormNextcloudFeedDetails|Feed added"));
  QCOMPARE(ttrss, QSL("FormTtRssFeedDetails|Feed added"));
#if defined(Q_OS_WIN)
  QCOMPARE(taskbar, QSL("WindowsTaskbar|Resume automatic feed fetching"));
#endif
}

void TestTranslations::generatorRowContexts() {
  FormMain window;
  ContextTranslator translator;
  QVERIFY(QCoreApplication::installTranslator(&translator));
  QString condition_text, condition_tooltip, action_tooltip;
  bool invoked = false;
  QTimer::singleShot(0, &window, [&]() {
    auto* manager = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    if (manager == nullptr) {
      return;
    }
    auto* add_filter = manager->findChild<QToolButton*>(QSL("m_btnAddNew"));
    if (add_filter != nullptr) {
      add_filter->click();
    }
    auto* generate = manager->findChild<QPushButton*>(QSL("m_btnGenerate"));
    if (generate == nullptr || !generate->isEnabled()) {
      manager->reject();
      return;
    }
    QTimer::singleShot(0, manager, [&]() {
      auto* generator = qobject_cast<QDialog*>(QApplication::activeModalWidget());
      if (generator == nullptr) {
        return;
      }
      for (auto* checkbox : generator->findChildren<QCheckBox*>()) {
        if (checkbox->text().endsWith(QSL("Case sensitive"))) {
          condition_text = checkbox->text();
        }
      }
      auto* add_action = generator->findChild<QPushButton*>(QSL("m_btnAddMatchingAction"));
      if (add_action != nullptr) {
        add_action->click();
      }
      for (auto* button : generator->findChildren<QToolButton*>()) {
        if (button->toolTip().endsWith(QSL("Remove condition"))) {
          condition_tooltip = button->toolTip();
        }
        if (button->toolTip().endsWith(QSL("Remove action"))) {
          action_tooltip = button->toolTip();
        }
      }
      generator->reject();
    });
    invoked = true;
    generate->click();
    manager->reject();
  });
  qApp->feedReader()->showMessageFiltersManager();
  QCoreApplication::removeTranslator(&translator);
  QVERIFY(invoked);
  QCOMPARE(condition_text, QSL("FormArticleFilterGenerator|Case sensitive"));
  QCOMPARE(condition_tooltip, QSL("FormArticleFilterGenerator|Remove condition"));
  QCOMPARE(action_tooltip, QSL("FormArticleFilterGenerator|Remove action"));
}

#if QT_VERSION_MAJOR >= 6
void TestTranslations::extractionCoverage() {
  QFile input(QString::fromUtf8(TEST_LUPDATE_INPUT));
  QVERIFY(input.open(QIODevice::ReadOnly | QIODevice::Text));
  const QStringList lines = QString::fromUtf8(input.readAll()).split(QL1C('\n'), Qt::SkipEmptyParts);
  const QString source_root = QString::fromUtf8(TEST_SOURCE_ROOT);
  QSet<QString> sources;
  for (const auto& line : lines) {
    if (line.startsWith(QSL("-I"))) {
      continue;
    }
    QVERIFY2(!sources.contains(line), qPrintable(line));
    QVERIFY2(isApplicationSource(source_root, line), qPrintable(line));
    sources.insert(line);
  }
  // These features need translations even when their build options are disabled.
  for (const auto& suffix : {QSL("/src/librssguard-xmpp/src/xmppnetwork.cpp"),
                             QSL("/src/librssguard/gui/webviewers/qtwebengine/webengineviewer.cpp"),
                             QSL("/src/librssguard/gui/messagetextbrowser.cpp"),
                             QSL("/src/librssguard/gui/mediaplayer/qtmultimedia/qtmultimediabackend.cpp"),
                             QSL("/src/librssguard-gmail/src/3rd-party/richtexteditor/mrichtextedit.cpp"),
                             QSL("/src/librssguard-gcalendar/src/gcalendarserviceroot.cpp"),
                             QSL("/src/librssguard-gcalendar/src/gui/gcalendaraccountdetails.cpp"),
                             QSL("/src/librssguard-gcalendar/src/gui/gcalendaraccountdetails.ui"),
                             QSL("/src/librssguard-gcalendar/src/gui/gcalendarreminderdetails.cpp"),
                             QSL("/src/librssguard-gcalendar/src/gui/gcalendarremindersettings.ui"),
                             QSL("/src/librssguard-gcalendar/src/gui/formgcalendarfeeddetails.cpp"),
                             QSL("/src/librssguard-gcalendar/src/gui/formcalendarevent.ui")}) {
    QVERIFY2(sources.contains(source_root + suffix), qPrintable(suffix));
  }

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString output_path = directory.filePath(QSL("rssguard_en.ts"));
  QByteArray previous_catalog;
  for (int pass = 0; pass < 2; ++pass) {
    QProcess process;
    process.start(QString::fromUtf8(TEST_CMAKE),
                  {QSL("-DLUPDATE=") + QString::fromUtf8(TEST_LUPDATE),
                   QSL("-DINPUT=") + input.fileName(),
                   QSL("-DOUTPUT=") + output_path,
                   QSL("-P"),
                   source_root + QSL("/localization/RunLupdate.cmake")});
    QVERIFY(process.waitForFinished(60000));
    const QByteArray output = process.readAllStandardOutput() + process.readAllStandardError();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
    const auto extracted = catalogKeys(output_path);
    QVERIFY(!extracted.isEmpty());
    QCOMPARE(extracted, catalogKeys(source_root + QSL("/localization/rssguard_en.ts")));
    for (const auto& source : {QSL("Welcome"), QSL("Go to changelog"), QSL("Unread articles fetched")}) {
      QVERIFY(extracted.contains(QStringList{QSL("Application"), source, {}, {}}.join(QChar(0x1f))));
    }
    for (const auto& key : {QStringList{QSL("GCalendarServiceRoot"), QSL("Authorize Google Calendar"), {}, {}},
                            QStringList{QSL("GCalendarAccountDetails"), QSL("Event cache"), {}, {}},
                            QStringList{QSL("GCalendarAccountDetails"), QSL("&Test setup"), {}, {}},
                            QStringList{QSL("GCalendarReminderDetails"), QSL("Multiple values"), {}, {}},
                            QStringList{QSL("GCalendarReminderSettings"),
                                        QSL("Apply reminder settings to all selected calendars"),
                                        {},
                                        {}},
                            QStringList{QSL("FormGCalendarFeedDetails"), QSL("Reminders"), {}, {}},
                            QStringList{QSL("FormCalendarEvent"), QSL("The whole series"), {}, {}}}) {
      QVERIFY2(extracted.contains(key.join(QChar(0x1f))), qPrintable(key.join(QL1C('|'))));
    }
    QFile catalog(output_path);
    QVERIFY(catalog.open(QIODevice::ReadOnly));
    const QByteArray current_catalog = catalog.readAll();
    if (pass > 0) {
      QCOMPARE(current_catalog, previous_catalog);
    }
    previous_catalog = current_catalog;
  }
}

void TestTranslations::failedUpdatesPreserveCatalog_data() {
  QTest::addColumn<bool>("existing");
  QTest::addColumn<bool>("missing_tool");
  QTest::newRow("warning-existing-catalog") << true << false;
  QTest::newRow("warning-new-catalog") << false << false;
  QTest::newRow("missing-tool-existing-catalog") << true << true;
  QTest::newRow("missing-tool-new-catalog") << false << true;
}

void TestTranslations::failedUpdatesPreserveCatalog() {
  QFETCH(bool, existing);
  QFETCH(bool, missing_tool);
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  QFile source(directory.filePath(QSL("warning.cpp")));
  QVERIFY(source.open(QIODevice::WriteOnly));
  source.write("void f() { object->tr(\"Missing context\"); }\n");
  source.close();
  QFile input(directory.filePath(QSL("input.txt")));
  QVERIFY(input.open(QIODevice::WriteOnly));
  input.write(source.fileName().toUtf8() + '\n');
  input.close();
  const QByteArray original = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                              "<TS version=\"2.1\" language=\"cs\"><context><name>Existing</name>"
                              "<message><source>Keep me</source><translation>Preserved translation</translation>"
                              "</message></context></TS>\n";
  QFile catalog(directory.filePath(QSL("warning.ts")));
  if (existing) {
    QVERIFY(catalog.open(QIODevice::WriteOnly));
    QCOMPARE(catalog.write(original), qint64(original.size()));
    catalog.close();
  }
  QProcess process;
  process.start(QString::fromUtf8(TEST_CMAKE),
                {QSL("-DLUPDATE=") +
                   (missing_tool ? directory.filePath(QSL("missing-lupdate")) : QString::fromUtf8(TEST_LUPDATE)),
                 QSL("-DINPUT=") + input.fileName(),
                 QSL("-DOUTPUT=") + catalog.fileName(),
                 QSL("-P"),
                 QString::fromUtf8(TEST_SOURCE_ROOT) + QSL("/localization/RunLupdate.cmake")});
  QVERIFY(process.waitForFinished(30000));
  const QByteArray output = process.readAllStandardOutput() + process.readAllStandardError();
  QVERIFY(process.exitCode() != 0);
  if (!missing_tool) {
    QVERIFY2(output.contains("Cannot invoke tr() like this"), output.constData());
  }
  if (existing) {
    QVERIFY(catalog.open(QIODevice::ReadOnly));
    QCOMPARE(catalog.readAll(), original);
  }
  else {
    QVERIFY(!catalog.exists());
  }
  QVERIFY(QDir(directory.path()).entryList({QSL(".lupdate-*")}, QDir::Files | QDir::Hidden).isEmpty());
}

void TestTranslations::checkoutPaths_data() {
  QTest::addColumn<QString>("checkout");
  QTest::newRow("ordinary") << QSL("ordinary/rssguard");
  QTest::newRow("under-build") << QSL("build/rssguard");
  QTest::newRow("under-third-party") << QSL("3rd-party/rssguard");
}

void TestTranslations::checkoutPaths() {
  QFETCH(QString, checkout);
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString root = directory.filePath(checkout);
  const QStringList expected{QSL("src/librssguard/core.cpp"),
                             QSL("src/librssguard-xmpp/src/disabled.cpp"),
                             QSL("src/librssguard-gcalendar/src/disabled.cpp"),
                             QSL("src/librssguard-gmail/src/3rd-party/richtexteditor/editor.cpp")};
  QStringList files = expected;
  files << QSL("src/librssguard/build/generated.cpp") << QSL("src/librssguard/CMakeFiles/generated.cpp")
        << QSL("src/librssguard/3rd-party/vendor.cpp") << QSL("src/librssguard-gmail/src/3rd-party/vendor.cpp")
        << QSL("src/librssguard-greader/src/3rd-party/richtexteditor/vendor.cpp");
  for (const auto& path : files) {
    QFile file(root + QL1C('/') + path);
    QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("void f() { QCoreApplication::translate(\"CheckoutFixture\", \"Covered\"); }\n");
  }
  QFile project(root + QSL("/CMakeLists.txt"));
  QVERIFY(project.open(QIODevice::WriteOnly));
  const QString cmake = QSL("cmake_minimum_required(VERSION 3.21.1)\n"
                            "project(translation_fixture NONE)\n"
                            "set(BUILD_GCALENDAR_PLUGIN OFF)\n"
                            "set(BUILD_GMAIL_PLUGIN OFF)\n"
                            "include(\"%1/localization/TranslationSources.cmake\")\n"
                            "rssguard_translation_sources(sources includes)\n"
                            "set(input \"\")\n"
                            "foreach(directory IN LISTS includes)\n"
                            "  string(APPEND input \"-I${directory}\\n\")\n"
                            "endforeach()\n"
                            "foreach(source IN LISTS sources)\n"
                            "  string(APPEND input \"${source}\\n\")\n"
                            "endforeach()\n"
                            "file(WRITE \"${CMAKE_BINARY_DIR}/input.txt\" \"${input}\")\n")
                          .arg(QString::fromUtf8(TEST_SOURCE_ROOT));
  project.write(cmake.toUtf8());
  project.close();
  const QString build = directory.filePath(QSL("output"));
  QProcess configure;
  QStringList arguments{QSL("-S"), root, QSL("-B"), build, QSL("-G"), QString::fromUtf8(TEST_CMAKE_GENERATOR)};
  if (!QString::fromUtf8(TEST_CMAKE_MAKE_PROGRAM).isEmpty()) {
    arguments << QSL("-DCMAKE_MAKE_PROGRAM=") + QString::fromUtf8(TEST_CMAKE_MAKE_PROGRAM);
  }
  configure.start(QString::fromUtf8(TEST_CMAKE), arguments);
  QVERIFY(configure.waitForFinished(30000));
  const QByteArray configure_output = configure.readAllStandardOutput() + configure.readAllStandardError();
  QVERIFY2(configure.exitCode() == 0, configure_output.constData());
  QFile input(build + QSL("/input.txt"));
  QVERIFY(input.open(QIODevice::ReadOnly | QIODevice::Text));
  QSet<QString> actual;
  for (const auto& line : QString::fromUtf8(input.readAll()).split(QL1C('\n'), Qt::SkipEmptyParts)) {
    if (!line.startsWith(QSL("-I"))) {
      QVERIFY2(isApplicationSource(root, line), qPrintable(line));
      actual.insert(QDir(root).relativeFilePath(line));
    }
  }
  QCOMPARE(actual, QSet<QString>(expected.begin(), expected.end()));
  QProcess extract;
  const QString catalog = build + QSL("/rssguard_en.ts");
  extract.start(QString::fromUtf8(TEST_CMAKE),
                {QSL("-DLUPDATE=") + QString::fromUtf8(TEST_LUPDATE),
                 QSL("-DINPUT=") + input.fileName(),
                 QSL("-DOUTPUT=") + catalog,
                 QSL("-P"),
                 QString::fromUtf8(TEST_SOURCE_ROOT) + QSL("/localization/RunLupdate.cmake")});
  QVERIFY(extract.waitForFinished(30000));
  const QByteArray extract_output = extract.readAllStandardOutput() + extract.readAllStandardError();
  QVERIFY2(extract.exitCode() == 0, extract_output.constData());
  const QSet<QString> expected_keys{QStringList{QSL("CheckoutFixture"), QSL("Covered"), {}, {}}.join(QChar(0x1f))};
  QCOMPARE(catalogKeys(catalog), expected_keys);
}

void TestTranslations::catalogDiscoveryIgnoresStaging() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString localization = directory.filePath(QSL("localization"));
  QVERIFY(QDir().mkpath(localization));
  QVERIFY(QFile::copy(QString::fromUtf8(TEST_SOURCE_ROOT) + QSL("/localization/CMakeLists.txt"),
                      localization + QSL("/CMakeLists.txt")));
  for (const auto& name : {QSL("rssguard_en.ts"),
                           QSL("rssguard_cs.ts"),
                           QSL("qtbase_cs.ts"),
                           QSL(".lupdate-active-rssguard_en.ts"),
                           QSL(".lupdate-interrupted-rssguard_cs.ts"),
                           QSL(".lupdate-active-qtbase_cs.ts"),
                           QSL("unrelated.ts")}) {
    QFile catalog(localization + QL1C('/') + name);
    QVERIFY(catalog.open(QIODevice::WriteOnly));
    catalog.write("<TS version=\"2.1\" language=\"en\"/>\n");
  }
  QFile project(directory.filePath(QSL("CMakeLists.txt")));
  QVERIFY(project.open(QIODevice::WriteOnly));
  // Run the real localization setup and capture the files passed to lrelease.
  project.write("cmake_minimum_required(VERSION 3.21.1)\n"
                "project(catalog_discovery NONE)\n"
                "set(BUILD_WITH_QT6 ON)\n"
                "set(UPDATE_TRANSLATIONS OFF)\n"
                "set(ENABLE_TESTING OFF)\n"
                "function(qt_add_lrelease target)\n"
                "  cmake_parse_arguments(arg \"\" \"QM_FILES_OUTPUT_VARIABLE\" \"TS_FILES;OPTIONS\" ${ARGN})\n"
                "  file(WRITE \"${CMAKE_BINARY_DIR}/catalogs.txt\" \"${arg_TS_FILES}\")\n"
                "endfunction()\n"
                "add_subdirectory(localization)\n");
  project.close();
  const QString build = directory.filePath(QSL("output"));
  QProcess configure;
  QStringList arguments{QSL("-S"),
                        directory.path(),
                        QSL("-B"),
                        build,
                        QSL("-G"),
                        QString::fromUtf8(TEST_CMAKE_GENERATOR)};
  if (!QString::fromUtf8(TEST_CMAKE_MAKE_PROGRAM).isEmpty()) {
    arguments << QSL("-DCMAKE_MAKE_PROGRAM=") + QString::fromUtf8(TEST_CMAKE_MAKE_PROGRAM);
  }
  configure.start(QString::fromUtf8(TEST_CMAKE), arguments);
  QVERIFY(configure.waitForFinished(30000));
  const QByteArray output = configure.readAllStandardOutput() + configure.readAllStandardError();
  QVERIFY2(configure.exitCode() == 0, output.constData());
  QFile catalogs(build + QSL("/catalogs.txt"));
  QVERIFY(catalogs.open(QIODevice::ReadOnly));
  const QStringList actual = QString::fromUtf8(catalogs.readAll()).split(QL1C(';'), Qt::SkipEmptyParts);
  const QSet<QString> expected{localization + QSL("/rssguard_en.ts"),
                               localization + QSL("/rssguard_cs.ts"),
                               localization + QSL("/qtbase_cs.ts")};
  QCOMPARE(QSet<QString>(actual.begin(), actual.end()), expected);
}
#endif

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
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseTrayIcon, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::AutoUpdateEnabled, false);
  settings.sync();
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        directory.path(),
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-translation-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  TestTranslations tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_translations.moc"
