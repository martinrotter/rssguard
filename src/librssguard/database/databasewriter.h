// For license of this file, see <project-root-folder>/LICENSE.md.
#pragma once

#include <functional>

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QSqlDatabase>
#include <QSqlError>
#include <QThread>
#include <QWaitCondition>

class DatabaseWriter : public QObject {
    Q_OBJECT
  public:
    struct WriteResult {
        bool m_success;
        QSqlError m_error;
    };

    explicit DatabaseWriter(QObject* parent = nullptr);
    ~DatabaseWriter();

    // New API: Blocking execution of arbitrary DB work
    WriteResult execWrite(std::function<WriteResult(const QSqlDatabase&)> func);

  private slots:
    void writerLoop();

  private:
    struct Job {
        std::function<WriteResult(const QSqlDatabase&)> m_func;
        WriteResult m_result;
        bool m_done = false;
        QWaitCondition m_doneCond;
    };

    void runJob(Job* job);

    QThread m_workerThread;
    QMutex m_queueMutex;
    QWaitCondition m_queueNotEmpty;
    QQueue<Job*> m_jobQueue;
};
