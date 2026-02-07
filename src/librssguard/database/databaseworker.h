// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DBWORKER_H
#define DBWORKER_H

#include "definitions/definitions.h"
#include "miscellaneous/thread.h"

#include <functional>

#include <QSqlDatabase>
#include <QThreadPool>
#include <QtConcurrent>

#define CONNECTION_READ  QSL("dbreader")
#define CONNECTION_WRITE QSL("dbwriter")

using DbReadFn = std::function<void(const QSqlDatabase&)>;
using DbWriteFn = std::function<void(const QSqlDatabase&)>;

class RSSGUARD_DLLSPEC DatabaseWorker : public QObject {
    Q_OBJECT

  public:
    explicit DatabaseWorker();
    virtual ~DatabaseWorker();

    template <typename T>
    T read(const std::function<T(const QSqlDatabase&)>& func);
    void read(const DbReadFn& func);

    template <typename T>
    T write(const std::function<T(const QSqlDatabase&)>& func);
    void write(const DbWriteFn& func);

  private:
    QSqlDatabase connectionForReading() const;
    QSqlDatabase connectionForWriting() const;

  private:
    QThreadPool m_readThreadPool;
    QThread m_writeThread;
    QSqlDatabase m_dbWriter;
};

template <typename T>
inline T DatabaseWorker::write(const std::function<T(const QSqlDatabase&)>& func) {
  T res;

  QMetaObject::invokeMethod(
    this,
    [&]() {
      if (!m_dbWriter.isValid()) {
        qDebugNN << LOGSEC_DB << "DB write setup job in thread" << NONQUOTE_W_SPACE_DOT(getThreadID());
        m_dbWriter = connectionForWriting();
      }

      qDebugNN << LOGSEC_DB << "DB write job in thread" << NONQUOTE_W_SPACE_DOT(getThreadID());

      res = func(m_dbWriter);
    },
    Qt::ConnectionType::BlockingQueuedConnection);

  return res;
}

template <typename T>
inline T DatabaseWorker::read(const std::function<T(const QSqlDatabase&)>& func) {
  QFuture<T> future = QtConcurrent::run(&m_readThreadPool, [&]() -> T {
    qDebugNN << LOGSEC_DB << "DB read job (with return) in thread" << NONQUOTE_W_SPACE_DOT(getThreadID());

    auto connection = connectionForReading();

    return func(connection);
  });

  future.waitForFinished();

  return future.result();
}

#endif // DBWORKER_H
