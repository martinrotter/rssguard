// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databaseworker.h"

#include "database/databasedriver.h"
#include "miscellaneous/application.h"

DatabaseWorker::DatabaseWorker() : QObject() {
  moveToThread(&m_writeThread);
  m_writeThread.start();

  connect(this, &DatabaseWorker::executeWrite, this, &DatabaseWorker::onExecuteWrite, Qt::BlockingQueuedConnection);
  m_readThreadPool.setMaxThreadCount(8);
  m_readThreadPool.setExpiryTimeout(-1);
}

DatabaseWorker::~DatabaseWorker() {
  m_readThreadPool.waitForDone(2000);
  m_writeThread.quit();
  m_writeThread.wait();
}

void DatabaseWorker::read(const DbReadFn& func) {
  QFuture<void> future = QtConcurrent::run(&m_readThreadPool, [&]() {
    qDebugNN << LOGSEC_DB << "DB read job in thread" << NONQUOTE_W_SPACE_DOT(QThread::currentThreadId());

    auto connection = connectionForReading();
    func(connection);
  });

  future.waitForFinished();
}

void DatabaseWorker::write(const DbWriteFn& func) {
  emit executeWrite(func);
}

void DatabaseWorker::onExecuteWrite(const DbWriteFn& func) {
  if (!m_dbWriter.isValid()) {
    qDebugNN << LOGSEC_DB << "DB write setup job in thread" << NONQUOTE_W_SPACE_DOT(QThread::currentThreadId());
    m_dbWriter = connectionForWriting();
  }

  qDebugNN << LOGSEC_DB << "DB write job in thread" << NONQUOTE_W_SPACE_DOT(QThread::currentThreadId());
  func(m_dbWriter);
}

QSqlDatabase DatabaseWorker::connectionForReading() const {
  return qApp->database()->driver()->threadSafeConnection(CONNECTION_READ);
}

QSqlDatabase DatabaseWorker::connectionForWriting() const {
  return qApp->database()->driver()->connection(CONNECTION_WRITE);
}
