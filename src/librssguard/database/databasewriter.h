// For license of this file, see <project-root-folder>/LICENSE.md.

#pragma once

#include "exceptions/applicationexception.h"

#include <atomic>
#include <functional>
#include <optional>

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QSqlDatabase>
#include <QThread>
#include <QWaitCondition>

class DatabaseWriter : public QObject {
    Q_OBJECT

  public:
    struct WriteResult {
        std::optional<ApplicationException> m_exception;
    };

    explicit DatabaseWriter(QObject* parent = nullptr);
    ~DatabaseWriter();

    // Blocking execution of arbitrary DB work.
    WriteResult execWrite(const std::function<void(const QSqlDatabase&)>& func);

    // Asynchronous execution of arbitrary DB work.
    //
    // NOTE: Callback always runs in the GUI thread.
    void execWriteAsync(const std::function<void(const QSqlDatabase&)>& func,
                        const std::function<void(const WriteResult&)>& callback = {});

  private slots:
    void writerLoop();

  private:
    struct Job {
        std::function<void(const QSqlDatabase&)> m_func;
        std::function<void(const WriteResult&)> m_callback;

        WriteResult m_result;
        bool m_done = false;
        bool m_blocking = false;
        QWaitCondition m_doneCond;
    };

    void runJob(Job* job);

  private:
    QThread m_workerThread;
    QMutex m_queueMutex;
    QWaitCondition m_queueNotEmpty;
    QQueue<Job*> m_jobQueue;

    QObject* m_guiDispatcher = nullptr;
    std::atomic_bool m_stop{false};
};
