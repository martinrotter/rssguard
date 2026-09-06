// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MEMORYDIAGNOSTICS_H
#define MEMORYDIAGNOSTICS_H

#include "definitions/definitions.h"

#include <atomic>

#include <QElapsedTimer>
#include <QObject>

class QTimer;

// Opt-in runtime diagnostics enabled by --memory-diagnostics. Keeping all
// counters and platform-specific process inspection here makes the feature
// easy to remove after the memory investigation is complete.
class RSSGUARD_DLLSPEC MemoryDiagnostics final : public QObject {
  public:
    explicit MemoryDiagnostics(QObject* parent = nullptr);
    ~MemoryDiagnostics() override;

    void start();
    void recordSnapshot(const QString& reason);

    static void requestSnapshot(const QString& reason, int delay_ms = 0);

    static quint64 registerWebPage(bool dummy_page);
    static void unregisterWebPage(quint64 page_id, bool dummy_page);
    static void reportWebPageRenderer(quint64 page_id, qint64 process_id);
    static void reportLingeringDummyPage(quint64 page_id);

    static void webLoadStarted();
    static void webLoadFinished(bool success);
    static void webContentRequestStarted();
    static void webContentRequestFinished(bool applied);

  private:
    static void updatePeak(std::atomic<quint64>& peak, quint64 current);

  private:
    static MemoryDiagnostics* s_instance;

    QTimer* m_snapshotTimer;
    QElapsedTimer m_elapsedTimer;
    quint64 m_snapshotSequence = 0;
    quint64 m_baselineTreePrivateBytes = 0;
    quint64 m_previousTreePrivateBytes = 0;

    std::atomic<quint64> m_nextWebPageId{0};
    std::atomic<quint64> m_createdWebPages{0};
    std::atomic<quint64> m_liveWebPages{0};
    std::atomic<quint64> m_liveDummyWebPages{0};
    std::atomic<quint64> m_webLoads{0};
    std::atomic<quint64> m_failedWebLoads{0};
    std::atomic<quint64> m_activeWebLoads{0};
    std::atomic<quint64> m_pendingWebContentRequests{0};
    std::atomic<quint64> m_peakPendingWebContentRequests{0};
    std::atomic<quint64> m_staleWebContentResults{0};
};

#endif // MEMORYDIAGNOSTICS_H
