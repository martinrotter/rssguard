// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/memorydiagnostics.h"

#include "definitions/definitions.h"

#include <algorithm>
#include <utility>

#include <QCoreApplication>
#include <QSet>
#include <QSqlDatabase>
#include <QStringList>
#include <QTimer>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

namespace {

constexpr int SNAPSHOT_INTERVAL_MS = 60000;

QString bytesToMib(quint64 bytes) {
  return QString::number(double(bytes) / (1024.0 * 1024.0), 'f', 1);
}

QString signedBytesToMib(qint64 bytes) {
  const QString value = QString::number(double(bytes) / (1024.0 * 1024.0), 'f', 1);

  return bytes >= 0 ? QL1C('+') + value : value;
}

#if defined(Q_OS_WIN)

struct ProcessSnapshot {
  DWORD m_processId = 0;
  DWORD m_parentProcessId = 0;
  DWORD m_threadCount = 0;
  DWORD m_handleCount = 0;
  DWORD m_queryError = ERROR_SUCCESS;
  QString m_executable;
  quint64 m_privateBytes = 0;
  quint64 m_workingSetBytes = 0;
  quint64 m_peakWorkingSetBytes = 0;
  bool m_memoryAvailable = false;
  bool m_handleCountAvailable = false;
};

QList<ProcessSnapshot> enumerateProcesses() {
  QList<ProcessSnapshot> processes;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (snapshot == INVALID_HANDLE_VALUE) {
    return processes;
  }

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);

  if (Process32FirstW(snapshot, &entry)) {
    do {
      ProcessSnapshot process;
      process.m_processId = entry.th32ProcessID;
      process.m_parentProcessId = entry.th32ParentProcessID;
      process.m_threadCount = entry.cntThreads;
      process.m_executable = QString::fromWCharArray(entry.szExeFile);
      processes.append(process);
    } while (Process32NextW(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return processes;
}

void populateProcessUsage(ProcessSnapshot& process, bool current_process) {
  HANDLE handle = current_process
                    ? GetCurrentProcess()
                    : OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process.m_processId);

  if (handle == nullptr) {
    process.m_queryError = GetLastError();
    return;
  }

  PROCESS_MEMORY_COUNTERS_EX memory{};
  memory.cb = sizeof(memory);

  if (GetProcessMemoryInfo(handle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
    process.m_privateBytes = quint64(memory.PrivateUsage);
    process.m_workingSetBytes = quint64(memory.WorkingSetSize);
    process.m_peakWorkingSetBytes = quint64(memory.PeakWorkingSetSize);
    process.m_memoryAvailable = true;
  }
  else {
    process.m_queryError = GetLastError();
  }

  DWORD handle_count = 0;

  if (GetProcessHandleCount(handle, &handle_count)) {
    process.m_handleCount = handle_count;
    process.m_handleCountAvailable = true;
  }
  else if (process.m_queryError == ERROR_SUCCESS) {
    process.m_queryError = GetLastError();
  }

  if (!current_process) {
    CloseHandle(handle);
  }
}

QList<ProcessSnapshot> processTreeForCurrentApplication() {
  QList<ProcessSnapshot> all_processes = enumerateProcesses();
  const DWORD own_process_id = GetCurrentProcessId();
  QSet<DWORD> tree_process_ids{own_process_id};
  bool found_new_descendant = false;

  do {
    found_new_descendant = false;

    for (const ProcessSnapshot& process : std::as_const(all_processes)) {
      if (!tree_process_ids.contains(process.m_processId) &&
          tree_process_ids.contains(process.m_parentProcessId)) {
        tree_process_ids.insert(process.m_processId);
        found_new_descendant = true;
      }
    }
  } while (found_new_descendant);

  QList<ProcessSnapshot> tree;

  for (ProcessSnapshot& process : all_processes) {
    if (!tree_process_ids.contains(process.m_processId)) {
      continue;
    }

    populateProcessUsage(process, process.m_processId == own_process_id);
    tree.append(std::move(process));
  }

  std::sort(tree.begin(), tree.end(), [](const ProcessSnapshot& left, const ProcessSnapshot& right) {
    return left.m_processId < right.m_processId;
  });

  return tree;
}

#endif

} // namespace

MemoryDiagnostics* MemoryDiagnostics::s_instance = nullptr;

MemoryDiagnostics::MemoryDiagnostics(QObject* parent) : QObject(parent), m_snapshotTimer(new QTimer(this)) {
  Q_ASSERT(s_instance == nullptr);
  s_instance = this;

  m_snapshotTimer->setInterval(SNAPSHOT_INTERVAL_MS);
  m_snapshotTimer->setTimerType(Qt::TimerType::VeryCoarseTimer);
  connect(m_snapshotTimer, &QTimer::timeout, this, [this]() {
    recordSnapshot(QSL("periodic"));
  });
}

MemoryDiagnostics::~MemoryDiagnostics() {
  m_snapshotTimer->stop();
  s_instance = nullptr;
}

void MemoryDiagnostics::start() {
  m_elapsedTimer.start();
  m_snapshotTimer->start();

  qInfoNN << LOGSEC_MEMORY << "Enabled; interval_ms=" << SNAPSHOT_INTERVAL_MS
          << ". This developmental feature only observes process state.";
  QTimer::singleShot(0, this, [this]() {
    recordSnapshot(QSL("diagnostics-started"));
  });
}

void MemoryDiagnostics::recordSnapshot(const QString& reason) {
  ++m_snapshotSequence;

  QStringList database_connections = QSqlDatabase::connectionNames();
  std::sort(database_connections.begin(), database_connections.end());

#if defined(Q_OS_WIN)
  const QList<ProcessSnapshot> process_tree = processTreeForCurrentApplication();
  const DWORD own_process_id = GetCurrentProcessId();
  const ProcessSnapshot* own_process = nullptr;
  quint64 descendants_private_bytes = 0;
  quint64 descendants_working_set_bytes = 0;
  int inaccessible_descendants = 0;
  int web_engine_descendants = 0;

  for (const ProcessSnapshot& process : process_tree) {
    if (process.m_processId == own_process_id) {
      own_process = &process;
      continue;
    }

    if (process.m_memoryAvailable) {
      descendants_private_bytes += process.m_privateBytes;
      descendants_working_set_bytes += process.m_workingSetBytes;
    }
    else {
      ++inaccessible_descendants;
    }

    if (process.m_executable.compare(QSL("QtWebEngineProcess.exe"), Qt::CaseInsensitive) == 0) {
      ++web_engine_descendants;
    }
  }

  const quint64 own_private_bytes = own_process != nullptr ? own_process->m_privateBytes : 0;
  const quint64 own_working_set_bytes = own_process != nullptr ? own_process->m_workingSetBytes : 0;
  const quint64 tree_private_bytes = own_private_bytes + descendants_private_bytes;
  const quint64 tree_working_set_bytes = own_working_set_bytes + descendants_working_set_bytes;

  if (m_baselineTreePrivateBytes == 0) {
    m_baselineTreePrivateBytes = tree_private_bytes;
  }

  const qint64 previous_delta = qint64(tree_private_bytes) - qint64(m_previousTreePrivateBytes);
  const qint64 baseline_delta = qint64(tree_private_bytes) - qint64(m_baselineTreePrivateBytes);
  m_previousTreePrivateBytes = tree_private_bytes;

  QStringList summary;
  summary << QSL("snapshot=%1").arg(m_snapshotSequence)
          << QSL("reason=\"%1\"").arg(reason)
          << QSL("elapsed_ms=%1").arg(m_elapsedTimer.elapsed())
          << QSL("self_pid=%1").arg(own_process_id)
          << QSL("self_private_mib=%1").arg(bytesToMib(own_private_bytes))
          << QSL("self_working_set_mib=%1").arg(bytesToMib(own_working_set_bytes))
          << QSL("self_peak_working_set_mib=%1")
               .arg(own_process != nullptr ? bytesToMib(own_process->m_peakWorkingSetBytes) : QSL("unavailable"))
          << QSL("self_handles=%1")
               .arg(own_process != nullptr && own_process->m_handleCountAvailable
                      ? QString::number(own_process->m_handleCount)
                      : QSL("unavailable"))
          << QSL("self_threads=%1").arg(own_process != nullptr ? own_process->m_threadCount : 0)
          << QSL("descendants=%1").arg(process_tree.isEmpty() ? 0 : process_tree.size() - 1)
          << QSL("webengine_descendants=%1").arg(web_engine_descendants)
          << QSL("inaccessible_descendants=%1").arg(inaccessible_descendants)
          << QSL("descendants_private_mib=%1").arg(bytesToMib(descendants_private_bytes))
          << QSL("descendants_working_set_mib=%1").arg(bytesToMib(descendants_working_set_bytes))
          << QSL("tree_private_mib=%1").arg(bytesToMib(tree_private_bytes))
          << QSL("tree_working_set_mib=%1").arg(bytesToMib(tree_working_set_bytes))
          << QSL("tree_private_previous_delta_mib=%1")
               .arg(m_snapshotSequence == 1 ? QSL("0.0") : signedBytesToMib(previous_delta))
          << QSL("tree_private_baseline_delta_mib=%1").arg(signedBytesToMib(baseline_delta))
          << QSL("db_connections=%1").arg(database_connections.size())
          << QSL("web_pages_created=%1").arg(m_createdWebPages.load())
          << QSL("web_pages_live=%1").arg(m_liveWebPages.load())
          << QSL("dummy_web_pages_live=%1").arg(m_liveDummyWebPages.load())
          << QSL("web_loads=%1").arg(m_webLoads.load())
          << QSL("failed_web_loads=%1").arg(m_failedWebLoads.load())
          << QSL("active_web_loads=%1").arg(m_activeWebLoads.load())
          << QSL("pending_web_content_requests=%1").arg(m_pendingWebContentRequests.load())
          << QSL("peak_pending_web_content_requests=%1").arg(m_peakPendingWebContentRequests.load())
          << QSL("stale_web_content_results=%1").arg(m_staleWebContentResults.load());

  qInfoNN << LOGSEC_MEMORY << summary.join(QL1C(' '));

  for (const ProcessSnapshot& process : process_tree) {
    if (process.m_processId == own_process_id) {
      continue;
    }

    QStringList detail;
    detail << QSL("snapshot=%1").arg(m_snapshotSequence) << QSL("process_pid=%1").arg(process.m_processId)
           << QSL("parent_pid=%1").arg(process.m_parentProcessId)
           << QSL("executable=\"%1\"").arg(process.m_executable)
           << QSL("private_mib=%1")
                .arg(process.m_memoryAvailable ? bytesToMib(process.m_privateBytes) : QSL("unavailable"))
           << QSL("working_set_mib=%1")
                .arg(process.m_memoryAvailable ? bytesToMib(process.m_workingSetBytes) : QSL("unavailable"))
           << QSL("peak_working_set_mib=%1")
                .arg(process.m_memoryAvailable ? bytesToMib(process.m_peakWorkingSetBytes) : QSL("unavailable"))
           << QSL("handles=%1")
                .arg(process.m_handleCountAvailable ? QString::number(process.m_handleCount) : QSL("unavailable"))
           << QSL("threads=%1").arg(process.m_threadCount)
           << QSL("query_error=%1").arg(process.m_queryError);

    qInfoNN << LOGSEC_MEMORY << detail.join(QL1C(' '));
  }
#else
  QStringList summary;
  summary << QSL("snapshot=%1").arg(m_snapshotSequence) << QSL("reason=\"%1\"").arg(reason)
          << QSL("elapsed_ms=%1").arg(m_elapsedTimer.elapsed()) << QSL("process_metrics=unsupported")
          << QSL("db_connections=%1").arg(database_connections.size())
          << QSL("web_pages_created=%1").arg(m_createdWebPages.load())
          << QSL("web_pages_live=%1").arg(m_liveWebPages.load())
          << QSL("dummy_web_pages_live=%1").arg(m_liveDummyWebPages.load())
          << QSL("web_loads=%1").arg(m_webLoads.load())
          << QSL("failed_web_loads=%1").arg(m_failedWebLoads.load())
          << QSL("active_web_loads=%1").arg(m_activeWebLoads.load())
          << QSL("pending_web_content_requests=%1").arg(m_pendingWebContentRequests.load())
          << QSL("peak_pending_web_content_requests=%1").arg(m_peakPendingWebContentRequests.load())
          << QSL("stale_web_content_results=%1").arg(m_staleWebContentResults.load());

  qInfoNN << LOGSEC_MEMORY << summary.join(QL1C(' '));
#endif

  qInfoNN << LOGSEC_MEMORY << "snapshot=" << m_snapshotSequence << " database_connection_names=\""
          << database_connections.join(QL1C(',')) << '"';
}

void MemoryDiagnostics::requestSnapshot(const QString& reason, int delay_ms) {
  MemoryDiagnostics* diagnostics = s_instance;

  if (diagnostics == nullptr) {
    return;
  }

  QTimer::singleShot(delay_ms, diagnostics, [diagnostics, reason]() {
    diagnostics->recordSnapshot(reason);
  });
}

quint64 MemoryDiagnostics::registerWebPage(bool dummy_page) {
  MemoryDiagnostics* diagnostics = s_instance;

  if (diagnostics == nullptr) {
    return 0;
  }

  const quint64 page_id = ++diagnostics->m_nextWebPageId;
  const quint64 live_pages = ++diagnostics->m_liveWebPages;
  ++diagnostics->m_createdWebPages;

  if (dummy_page) {
    ++diagnostics->m_liveDummyWebPages;
  }

  qInfoNN << LOGSEC_MEMORY << "event=web-page-created page_id=" << page_id << " dummy=" << dummy_page
          << " live_pages=" << live_pages << " live_dummy_pages=" << diagnostics->m_liveDummyWebPages.load();
  return page_id;
}

void MemoryDiagnostics::unregisterWebPage(quint64 page_id, bool dummy_page) {
  MemoryDiagnostics* diagnostics = s_instance;

  if (diagnostics == nullptr || page_id == 0) {
    return;
  }

  const quint64 live_pages = --diagnostics->m_liveWebPages;

  if (dummy_page) {
    --diagnostics->m_liveDummyWebPages;
  }

  qInfoNN << LOGSEC_MEMORY << "event=web-page-destroyed page_id=" << page_id << " dummy=" << dummy_page
          << " live_pages=" << live_pages << " live_dummy_pages=" << diagnostics->m_liveDummyWebPages.load();
}

void MemoryDiagnostics::reportWebPageRenderer(quint64 page_id, qint64 process_id) {
  if (s_instance != nullptr && page_id != 0) {
    qInfoNN << LOGSEC_MEMORY << "event=web-page-renderer-changed page_id=" << page_id
            << " renderer_pid=" << process_id;
  }
}

void MemoryDiagnostics::reportLingeringDummyPage(quint64 page_id) {
  if (s_instance != nullptr && page_id != 0) {
    qWarningNN << LOGSEC_MEMORY << "event=dummy-web-page-still-alive page_id=" << page_id
               << " age_ms=30000. The popup page did not finish URL handoff.";
    requestSnapshot(QSL("dummy-web-page-still-alive"));
  }
}

void MemoryDiagnostics::webLoadStarted() {
  if (s_instance != nullptr) {
    ++s_instance->m_activeWebLoads;
  }
}

void MemoryDiagnostics::webLoadFinished(bool success) {
  MemoryDiagnostics* diagnostics = s_instance;

  if (diagnostics == nullptr) {
    return;
  }

  ++diagnostics->m_webLoads;

  if (!success) {
    ++diagnostics->m_failedWebLoads;
  }

  if (diagnostics->m_activeWebLoads.load() > 0) {
    --diagnostics->m_activeWebLoads;
  }
}

void MemoryDiagnostics::webContentRequestStarted() {
  MemoryDiagnostics* diagnostics = s_instance;

  if (diagnostics == nullptr) {
    return;
  }

  const quint64 pending = ++diagnostics->m_pendingWebContentRequests;
  updatePeak(diagnostics->m_peakPendingWebContentRequests, pending);
}

void MemoryDiagnostics::webContentRequestFinished(bool applied) {
  MemoryDiagnostics* diagnostics = s_instance;

  if (diagnostics == nullptr) {
    return;
  }

  if (diagnostics->m_pendingWebContentRequests.load() > 0) {
    --diagnostics->m_pendingWebContentRequests;
  }

  if (!applied) {
    ++diagnostics->m_staleWebContentResults;
  }
}

void MemoryDiagnostics::updatePeak(std::atomic<quint64>& peak, quint64 current) {
  quint64 previous_peak = peak.load();

  while (previous_peak < current && !peak.compare_exchange_weak(previous_peak, current)) {
  }
}
