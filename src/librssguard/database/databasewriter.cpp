// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasewriter.h"

#include "database/databasedriver.h"
#include "miscellaneous/application.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#define CONNECTION_NAME QSL("db_writer")

DatabaseWriter::DatabaseWriter(QObject* parent) : QObject(parent) {
  m_guiDispatcher = qApp;

  m_workerThread.start();
  moveToThread(&m_workerThread);

  // Create the DB connection inside the writer thread.
  QMetaObject::invokeMethod(
    this,
    []() {
      qApp->database()->driver()->connection(CONNECTION_NAME);
    },
    Qt::ConnectionType::QueuedConnection);

  // Start the main job loop in the writer thread
  QMetaObject::invokeMethod(this, "writerLoop", Qt::ConnectionType::QueuedConnection);
}

DatabaseWriter::~DatabaseWriter() {
  {
    QMutexLocker locker(&m_queueMutex);
    m_stop = true;
    m_queueNotEmpty.wakeAll();
  }

  m_workerThread.quit();
  m_workerThread.wait();
}

DatabaseWriter::WriteResult DatabaseWriter::execWrite(const std::function<void(const QSqlDatabase&)>& func) {
  Job job;
  job.m_func = func;
  job.m_blocking = true;

  {
    QMutexLocker locker(&m_queueMutex);

    if (m_stop) {
      WriteResult res;
      res.m_exception = ApplicationException(tr("database writer is stopping"));
      return res;
    }

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

void DatabaseWriter::execWriteAsync(const std::function<void(const QSqlDatabase&)>& func,
                                    const std::function<void(const WriteResult&)>& callback) {
  Job* job = new Job;
  job->m_func = func;
  job->m_callback = callback;
  job->m_blocking = false;

  {
    QMutexLocker locker(&m_queueMutex);

    if (m_stop) {
      WriteResult res;
      res.m_exception = ApplicationException(tr("database writer is stopping"));

      if (callback) {
        QMetaObject::invokeMethod(
          m_guiDispatcher,
          [callback, res]() {
            callback(res);
          },
          Qt::ConnectionType::QueuedConnection);
      }

      delete job;
      return;
    }

    m_jobQueue.enqueue(job);
    m_queueNotEmpty.wakeOne();
  }
}

void DatabaseWriter::writerLoop() {
  while (true) {
    Job* job = nullptr;

    {
      QMutexLocker locker(&m_queueMutex);

      while (m_jobQueue.isEmpty() && !m_stop) {
        m_queueNotEmpty.wait(&m_queueMutex);
      }

      if (m_stop && m_jobQueue.isEmpty()) {
        break;
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

  try {
    job->m_func(db);
    job->m_result.m_exception = std::nullopt;
  }
  catch (const ApplicationException& ex) {
    job->m_result.m_exception = std::optional<ApplicationException>(ex);
  }

  if (job->m_blocking) {
    // Synchronous call.
    job->m_done = true;
    job->m_doneCond.wakeOne();
  }
  else {
    // Async: deliver callback in GUI thread.
    if (job->m_callback) {
      WriteResult resultCopy = job->m_result;

      QMetaObject::invokeMethod(
        m_guiDispatcher,
        [cb = job->m_callback, resultCopy]() {
          cb(resultCopy);
        },
        Qt::ConnectionType::QueuedConnection);
    }

    delete job;
  }
}
