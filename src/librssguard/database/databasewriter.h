// For license of this file, see <project-root-folder>/LICENSE.md.

#pragma once

#include <functional>

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QWaitCondition>

class DatabaseWriter : public QObject {
    Q_OBJECT
  public:
    struct WriteResult {
        bool success;
        QString error;
    };

    explicit DatabaseWriter(QObject* parent = nullptr);
    ~DatabaseWriter();

    // Blocking write
    WriteResult execWrite(const QString& sql, const QVariantList& params = {});

  private slots:
    void writerLoop();

  private:
    struct Job {
        QString sql;
        QVariantList params;
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
