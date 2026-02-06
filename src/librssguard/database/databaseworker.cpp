// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databaseworker.h"

#include "database/databasedriver.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QtConcurrent>

#define CONNECTION_READ  QSL("dbreader")
#define CONNECTION_WRITE QSL("dbwriter")

DatabaseWorker::DatabaseWorker(QObject* parent) : QObject(parent) {
  moveToThread(&m_writeThread);
  connect(this, &DatabaseWorker::executeWrite, this, &DatabaseWorker::onExecuteWrite, Qt::BlockingQueuedConnection);
  m_writeThread.start();
  m_readThreadPool.setMaxThreadCount(8);
  m_readThreadPool.setExpiryTimeout(-1);

  QMetaObject::invokeMethod(
    this,
    [this]() {
      qDebugNN << LOGSEC_DB << "DB write setup job in thread" << NONQUOTE_W_SPACE_DOT(QThread::currentThreadId());

      m_dbWriter = qApp->database()->driver()->connection(CONNECTION_WRITE);
    },
    Qt::ConnectionType::BlockingQueuedConnection);
}

DatabaseWorker::~DatabaseWorker() {
  m_readThreadPool.waitForDone(2000);
  m_writeThread.quit();
  m_writeThread.wait();
}

void DatabaseWorker::read(const std::function<void(QSqlDatabase)>& func) {
  QFuture<void> future = QtConcurrent::run(&m_readThreadPool, [&]() {
    qDebugNN << LOGSEC_DB << "DB read job in thread" << NONQUOTE_W_SPACE_DOT(QThread::currentThreadId());

    auto connection = qApp->database()->driver()->threadSafeConnection(CONNECTION_READ);
    func(connection);
  });

  future.waitForFinished();
}

void DatabaseWorker::write(const std::function<void(QSqlDatabase)>& func) {
  emit executeWrite(func);
}

void DatabaseWorker::onExecuteWrite(const std::function<void(QSqlDatabase)>& func) {
  qDebugNN << LOGSEC_DB << "DB write job in thread" << NONQUOTE_W_SPACE_DOT(QThread::currentThreadId());

  func(m_dbWriter);
}
