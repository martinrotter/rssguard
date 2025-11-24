// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasewriter.h"

#include "database/databasedriver.h"
#include "miscellaneous/application.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#define CONNECTION_NAME QSL("db_writer")

DatabaseWriter::DatabaseWriter(QObject* parent) : QObject(parent) {
  m_workerThread.start();
  moveToThread(&m_workerThread);

  // Create connection inside writer thread
  QMetaObject::invokeMethod(this, []() {
    qApp->database()->driver()->connection(CONNECTION_NAME);
  });

  // Start writer loop
  QMetaObject::invokeMethod(this, "writerLoop", Qt::ConnectionType::QueuedConnection);
}

DatabaseWriter::~DatabaseWriter() {
  m_workerThread.quit();
  m_workerThread.wait();
}

DatabaseWriter::WriteResult DatabaseWriter::execWrite(std::function<WriteResult(const QSqlDatabase&)> func) {
  Job job;
  job.m_func = func;

  {
    QMutexLocker locker(&m_queueMutex);
    m_jobQueue.enqueue(&job);
    m_queueNotEmpty.wakeOne();
  }

  QMutex local;
  local.lock();

  while (!job.m_done) {
    job.m_doneCond.wait(&local);
  }

  local.unlock();

  return job.m_result;
}

void DatabaseWriter::writerLoop() {
  while (true) {
    Job* job = nullptr;

    {
      QMutexLocker locker(&m_queueMutex);

      if (m_jobQueue.isEmpty()) {
        m_queueNotEmpty.wait(&m_queueMutex);
      }

      if (!m_jobQueue.isEmpty()) {
        job = m_jobQueue.dequeue();
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
  job->m_result = job->m_func(db);

  // Notify waiting thread
  job->m_done = true;
  job->m_doneCond.wakeOne();
}
