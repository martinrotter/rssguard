// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasewriter.h"

#include "database/databasedriver.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#define CONNECTION_NAME QSL("db_writer")

DatabaseWriter::DatabaseWriter(QObject* parent) : QObject(parent) {
  workerThread.start();
  moveToThread(&workerThread);

  QMetaObject::invokeMethod(this, []() {
    // NOTE: Initialize connection.
    qApp->database()->driver()->connection(CONNECTION_NAME);
  });

  QMetaObject::invokeMethod(this, "writerLoop", Qt::ConnectionType::QueuedConnection);
}

DatabaseWriter::~DatabaseWriter() {
  workerThread.quit();
  workerThread.wait();
}

DatabaseWriter::WriteResult DatabaseWriter::execWrite(const QString& sql, const QVariantList& params) {
  Job job;
  job.sql = sql;
  job.params = params;

  // Add job to queue.
  {
    QMutexLocker locker(&queueMutex);
    jobQueue.enqueue(&job);
    queueNotEmpty.wakeOne();
  }

  // Block until job finishes.
  QMutex localMutex;

  localMutex.lock();

  while (!job.done) {
    job.doneCond.wait(&localMutex);
  }

  localMutex.unlock();

  return job.result;
}

void DatabaseWriter::writerLoop() {
  while (true) {
    Job* job = nullptr;

    {
      QMutexLocker locker(&queueMutex);

      if (jobQueue.isEmpty()) {
        queueNotEmpty.wait(&queueMutex);
      }

      if (!jobQueue.isEmpty()) {
        job = jobQueue.dequeue();
      }
    }

    if (job) {
      runJob(job);
    }
  }
}

void DatabaseWriter::runJob(Job* job) {
  QSqlDatabase db = qApp->database()->driver()->connection(CONNECTION_NAME);

  SqlQuery query(db);
  query.prepare(job->sql);

  for (int i = 0; i < job->params.size(); ++i) {
    query.bindValue(i, job->params[i]);
  }

  bool ok = query.exec();

  job->result.success = ok;
  job->result.error = ok ? QString() : query.lastError().text();

  // Mark job complete.
  job->done = true;
  job->doneCond.wakeOne();
}
