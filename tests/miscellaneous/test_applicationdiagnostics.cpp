// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"
#include "miscellaneous/memorydiagnostics.h"
#include "miscellaneous/settingskeys.h"

#include <mutex>

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {
  constexpr int fatal_handler_returned = 42;

#if defined(Q_OS_WIN)
  std::mutex messages_mutex;
  QStringList captured_messages;

  void captureMessage(QtMsgType, const QMessageLogContext&, const QString& message) {
    const std::lock_guard<std::mutex> lock(messages_mutex);
    captured_messages.append(message);
  }

  class MessageCapture {
    public:
      MessageCapture() {
        captured_messages.clear();
        m_previousHandler = qInstallMessageHandler(captureMessage);
      }

      ~MessageCapture() {
        qInstallMessageHandler(m_previousHandler);
      }

      QString snapshot(const QString& reason) const {
        const std::lock_guard<std::mutex> lock(messages_mutex);
        for (const QString& message : captured_messages) {
          if (message.contains(QStringLiteral("reason=\"%1\"").arg(reason))) {
            return message;
          }
        }
        return {};
      }

    private:
      QtMessageHandler m_previousHandler;
  };

  struct NativeResources {
      HDC m_dc = CreateCompatibleDC(nullptr);
      HBITMAP m_bitmap = CreateBitmap(2, 2, 1, 32, nullptr);
      HWND m_window = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 2, 2, nullptr, nullptr, nullptr, nullptr);

      ~NativeResources() {
        if (m_window != nullptr) {
          DestroyWindow(m_window);
        }
        if (m_bitmap != nullptr) {
          DeleteObject(m_bitmap);
        }
        if (m_dc != nullptr) {
          DeleteDC(m_dc);
        }
      }
  };

  qint64 metric(const QString& snapshot, const QString& name) {
    const auto match = QRegularExpression(QStringLiteral("\\b%1=(\\d+)\\b").arg(name)).match(snapshot);
    return match.hasMatch() ? match.captured(1).toLongLong() : -1;
  }
#endif
} // namespace

class TestApplicationDiagnostics : public QObject {
    Q_OBJECT

  private slots:
    void startupLogging_data();
    void startupLogging();
    void fatalMessageHandlerReturns();
    void recordsGuiResourceCountsAndPeaks();
};

void TestApplicationDiagnostics::startupLogging_data() {
  QTest::addColumn<QStringList>("options");
  QTest::addColumn<QVariant>("disable_debug");
  QTest::addColumn<bool>("verbose");

  QTest::newRow("default") << QStringList() << QVariant() << false;
  QTest::newRow("saved-disabled") << QStringList() << QVariant(true) << false;
  QTest::newRow("saved-enabled") << QStringList() << QVariant(false) << true;
  QTest::newRow("debug-short") << QStringList{QStringLiteral("-g")} << QVariant() << true;
  QTest::newRow("debug-override") << QStringList{QStringLiteral("--debug")} << QVariant(true) << true;
  QTest::newRow("memory-diagnostics") << QStringList{QStringLiteral("--memory-diagnostics")} << QVariant(true) << true;
}

void TestApplicationDiagnostics::startupLogging() {
  QFETCH(QStringList, options);
  QFETCH(QVariant, disable_debug);
  QFETCH(bool, verbose);

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString config_directory = directory.filePath(QSL(APP_CFG_PATH));
  QVERIFY(QDir().mkpath(config_directory));
  QSettings settings(QDir(config_directory).filePath(QSL(APP_CFG_FILE)), QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  if (disable_debug.isValid()) {
    settings.setValue(GROUP(General) + QSL("/") + General::DisableDebugOutput, disable_debug);
  }
  settings.sync();
  QCOMPARE(settings.status(), QSettings::NoError);

  QStringList arguments{QStringLiteral("--startup-log-probe"),
                        QStringLiteral("--data"),
                        directory.path(),
                        QStringLiteral("--force-text-browser")};
  arguments.append(options);
  QProcess child;
  child.start(QCoreApplication::applicationFilePath(), arguments);
  QVERIFY2(child.waitForStarted(), qPrintable(child.errorString()));
  QVERIFY2(child.waitForFinished(), qPrintable(child.errorString()));
  const QByteArray output = child.readAllStandardError();
  QCOMPARE(child.exitStatus(), QProcess::NormalExit);
  QVERIFY2(child.exitCode() == 0, output.constData());
  QVERIFY2(!output.contains("bootstrap-debug-probe"), output.constData());
  QVERIFY2(!output.contains("bootstrap-info-probe"), output.constData());
  QVERIFY2(!output.contains("Initializing settings in"), output.constData());
  QVERIFY2(output.contains("bootstrap-warning-probe"), output.constData());
  QCOMPARE(output.contains("Starting to load active localization."), verbose);
  QCOMPARE(output.contains("initialized-debug-probe"), verbose);
  QCOMPARE(output.contains("initialized-info-probe"), verbose);
  QVERIFY2(output.contains("initialized-warning-probe"), output.constData());
  QVERIFY2(output.contains("initialized-critical-probe"), output.constData());
}

void TestApplicationDiagnostics::fatalMessageHandlerReturns() {
  QProcess child;
  child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--fatal-handler-probe")});
  QVERIFY2(child.waitForStarted(), qPrintable(child.errorString()));
  QVERIFY2(child.waitForFinished(), qPrintable(child.errorString()));
  QCOMPARE(child.exitStatus(), QProcess::NormalExit);
  QCOMPARE(child.exitCode(), fatal_handler_returned);
}

void TestApplicationDiagnostics::recordsGuiResourceCountsAndPeaks() {
#if defined(Q_OS_WIN)
  QString baseline;
  QString allocated;
  QString released;
  bool resources_created = false;
  {
    MessageCapture capture;
    MemoryDiagnostics diagnostics;
    diagnostics.start();
    diagnostics.recordSnapshot(QStringLiteral("baseline"));
    baseline = capture.snapshot(QStringLiteral("baseline"));
    {
      NativeResources resources;
      resources_created = resources.m_dc != nullptr && resources.m_bitmap != nullptr && resources.m_window != nullptr;
      diagnostics.recordSnapshot(QStringLiteral("allocated"));
      allocated = capture.snapshot(QStringLiteral("allocated"));
    }
    diagnostics.recordSnapshot(QStringLiteral("released"));
    released = capture.snapshot(QStringLiteral("released"));
  }

  QVERIFY(resources_created);
  QVERIFY2(!baseline.isEmpty(), "The baseline diagnostics snapshot was not logged.");
  QVERIFY2(!allocated.isEmpty(), "The allocation diagnostics snapshot was not logged.");
  QVERIFY2(!released.isEmpty(), "The release diagnostics snapshot was not logged.");
  for (const QString& name : {QStringLiteral("gdi"), QStringLiteral("user")}) {
    const QString current = QStringLiteral("self_%1_objects").arg(name);
    const QString peak = QStringLiteral("self_peak_%1_objects").arg(name);
    QVERIFY(metric(baseline, current) >= 0);
    QVERIFY(metric(baseline, peak) >= metric(baseline, current));
    QVERIFY(metric(allocated, current) > metric(baseline, current));
    QVERIFY(metric(allocated, peak) >= metric(allocated, current));
    QVERIFY(metric(released, current) < metric(allocated, current));
    QVERIFY(metric(released, peak) >= metric(allocated, peak));
  }
#else
  QSKIP("GDI and USER resource counts are only available on Windows.");
#endif
}

int main(int argc, char* argv[]) {
  if (argc > 1 && QByteArray(argv[1]) == "--startup-log-probe") {
    qInstallMessageHandler(Application::performLogging);
    qDebug("bootstrap-debug-probe");
    qInfo("bootstrap-info-probe");
    qWarning("bootstrap-warning-probe");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QStringList arguments{QString::fromLocal8Bit(argv[0])};
    for (int i = 2; i < argc; ++i) {
      arguments.append(QString::fromLocal8Bit(argv[i]));
    }
    Application application(QStringLiteral("rssguard-startup-log-test"), argc, argv, arguments);
    qDebug("initialized-debug-probe");
    qInfo("initialized-info-probe");
    qWarning("initialized-warning-probe");
    qCritical("initialized-critical-probe");
    return 0;
  }
  QCoreApplication application(argc, argv);
  if (application.arguments().contains(QStringLiteral("--fatal-handler-probe"))) {
    Application::performLogging(QtFatalMsg, QMessageLogContext(), QStringLiteral("fatal-handler-probe"));
    return fatal_handler_returned;
  }
  TestApplicationDiagnostics tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_applicationdiagnostics.moc"
