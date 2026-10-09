// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/application.h"
#include "miscellaneous/memorydiagnostics.h"

#include <mutex>

#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>
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
    void fatalMessageHandlerReturns();
    void recordsGuiResourceCountsAndPeaks();
};

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
  QCoreApplication application(argc, argv);
  if (application.arguments().contains(QStringLiteral("--fatal-handler-probe"))) {
    Application::performLogging(QtFatalMsg, QMessageLogContext(), QStringLiteral("fatal-handler-probe"));
    return fatal_handler_returned;
  }
  TestApplicationDiagnostics tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_applicationdiagnostics.moc"
