// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasewriter.h"

#include "database/databasedriver.h"
#include "miscellaneous/application.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#define CONNECTION_NAME QSL("db_writer")

DatabaseWriter::DatabaseWriter(QObject* parent) : QObject(parent) {
  workerThread.start();
  moveToThread(&workerThread);

  // Create connection inside writer thread
  QMetaObject::invokeMethod(this, []() {
    qApp->database()->driver()->connection(CONNECTION_NAME);
  });

  // Start writer loop
  QMetaObject::invokeMethod(this, "writerLoop", Qt::QueuedConnection);
}

DatabaseWriter::~DatabaseWriter() {
  workerThread.quit();
  workerThread.wait();
}

DatabaseWriter::WriteResult DatabaseWriter::execWrite(std::function<WriteResult(const QSqlDatabase&)> func) {
  Job job;
  job.func = func;

  {
    QMutexLocker locker(&queueMutex);
    jobQueue.enqueue(&job);
    queueNotEmpty.wakeOne();
  }

  QMutex local;
  local.lock();

  while (!job.done) {
    job.doneCond.wait(&local);
  }

  local.unlock();

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

  // Execute user function
  job->result = job->func(db);

  // Notify waiting thread
  job->done = true;
  job->doneCond.wakeOne();
}
