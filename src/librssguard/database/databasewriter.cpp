// For license of this file, see <project-root-folder>/LICENSE.md.
#include "database/databasewriter.h"

#include "database/databasedriver.h"
#include "exceptions/sqlexception.h"
#include "miscellaneous/application.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#define CONNECTION_NAME_READ  QSL("db_reader")
#define CONNECTION_NAME_WRITE QSL("db_writer")

DatabaseWriter::DatabaseWriter(QObject* parent) : QObject(parent) {
  m_guiDispatcher = qApp;

  moveToThread(&m_workerThread);
  m_workerThread.start();

  // Create the DB connection inside the writer thread.
  QMetaObject::invokeMethod(
    this,
    []() {
      qApp->database()->driver()->connection(CONNECTION_NAME_WRITE);
    },
    Qt::ConnectionType::QueuedConnection);

  // Start the main job loop in the writer thread
  QMetaObject::invokeMethod(this, "writerLoop", Qt::ConnectionType::QueuedConnection);
}

DatabaseWriter::~DatabaseWriter() {
  // Set stop flag first.
  m_stop = true;

  // Clear queue and wake up any waiting threads.
  {
    QMutexLocker locker(&m_queueMutex);

    // Process all queued jobs.
    while (!m_jobQueue.isEmpty()) {
      Job* job = m_jobQueue.dequeue();

      if (job->m_blocking) {
        // Wake up blocking caller with an exception.
        {
          QMutexLocker doneLocker(&job->m_doneMutex);
          job->m_result.m_exception = ApplicationException(tr("database writer is stopping"));
          job->m_done = true;
        }
        job->m_doneCond.wakeOne();

        // Don't delete - it's stack-allocated in execWrite.
      }
      else {
        // Just delete async jobs without executing callbacks.
        delete job;
      }
    }

    // Wake up the worker thread so it can exit.
    m_queueNotEmpty.wakeAll();
  }

  m_workerThread.quit();
  m_workerThread.wait();
}

void DatabaseWriter::execRead(const std::function<void(const QSqlDatabase&)>& func) {
  QSqlDatabase db = qApp->database()->driver()->threadSafeConnection(CONNECTION_NAME_READ);

  if (!db.isOpen()) {
    throw SqlException(db.lastError());
  }

  func(db);
}

void DatabaseWriter::execWrite(const std::function<void(const QSqlDatabase&)>& func) {
  Job job;
  job.m_func = func;
  job.m_blocking = true;

  {
    QMutexLocker locker(&m_queueMutex);
    if (m_stop) {
      throw ApplicationException(tr("database writer is stopping"));
    }

    m_jobQueue.enqueue(&job);
    m_queueNotEmpty.wakeOne();
  }

  QMutexLocker locker(&job.m_doneMutex);
  while (!job.m_done) {
    job.m_doneCond.wait(&job.m_doneMutex);
  }

  if (job.m_result.m_exception.has_value()) {
    throw job.m_result.m_exception;
  }
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
      // Don't execute callback during shutdown, just delete and return
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

      // Exit immediately if stop is set
      if (m_stop) {
        break;
      }

      if (!m_jobQueue.isEmpty()) {
        job = m_jobQueue.dequeue();
      }
    }

    if (job) {
      // Double-check stop flag before running job
      if (m_stop) {
        if (job->m_blocking) {
          // Wake up blocking caller with exception
          {
            QMutexLocker locker(&job->m_doneMutex);
            job->m_result.m_exception = ApplicationException(tr("database writer is stopping"));
            job->m_done = true;
          }
          job->m_doneCond.wakeOne();
        }
        else {
          delete job;
        }
      }
      else {
        runJob(job);
      }
    }
  }
}

void DatabaseWriter::runJob(Job* job) {
  try {
    QSqlDatabase db = qApp->database()->driver()->connection(CONNECTION_NAME_WRITE);

    if (!db.isOpen()) {
      throw SqlException(db.lastError());
    }

    job->m_func(db);
    job->m_result.m_exception = std::nullopt;
  }
  catch (const ApplicationException& ex) {
    job->m_result.m_exception = std::optional<ApplicationException>(ex);
  }

  if (job->m_blocking) {
    {
      QMutexLocker locker(&job->m_doneMutex);
      job->m_done = true;
    }
    job->m_doneCond.wakeOne();
  }
  else {
    // Only execute callback if not stopping
    if (!m_stop && job->m_callback) {
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
