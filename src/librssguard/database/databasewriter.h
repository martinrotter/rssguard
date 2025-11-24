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
        bool success;
        QSqlError error;
    };

    explicit DatabaseWriter(QObject* parent = nullptr);
    ~DatabaseWriter();

    // New API: Blocking execution of arbitrary DB work
    WriteResult execWrite(std::function<WriteResult(const QSqlDatabase&)> func);

  private slots:
    void writerLoop();

  private:
    struct Job {
        std::function<WriteResult(const QSqlDatabase&)> func;
        WriteResult result;
        bool done = false;
        QWaitCondition doneCond;
    };

    QThread workerThread;

    QMutex queueMutex;
    QWaitCondition queueNotEmpty;
    QQueue<Job*> jobQueue;

    void runJob(Job* job);
};
