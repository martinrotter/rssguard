// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DBWORKER_H
#define DBWORKER_H

#include "definitions/definitions.h"
#include "miscellaneous/thread.h"

#include <exception>
#include <functional>
#include <optional>

#include <QSqlDatabase>
#include <QThreadPool>
#include <QtConcurrent>

#define CONNECTION_READ  QSL("dbreader")
#define CONNECTION_WRITE QSL("dbwriter")

using DbReadFn = std::function<void(const QSqlDatabase&)>;
using DbWriteFn = std::function<void(const QSqlDatabase&)>;

class DatabaseDriver;

class RSSGUARD_DLLSPEC DatabaseWorker : public QObject {
    Q_OBJECT

  public:
    explicit DatabaseWorker(DatabaseDriver* driver);
    virtual ~DatabaseWorker();

    void shutdown();

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
    DatabaseDriver* m_driver;
    QThreadPool m_readThreadPool;
    QThread m_writeThread;
    QSqlDatabase m_dbWriter;
};

template <typename T>
inline T DatabaseWorker::write(const std::function<T(const QSqlDatabase&)>& func) {
  T res;
  std::exception_ptr eptr = nullptr;

  QMetaObject::invokeMethod(
    this,
    [&]() {
      if (!m_dbWriter.isValid()) {
        qDebugNN << LOGSEC_DB << "DB write setup job in thread" << NONQUOTE_W_SPACE_DOT(getThreadID());
      }

      try {
        m_dbWriter = connectionForWriting();

        qDebugNN << LOGSEC_DB << "DB write job in thread" << NONQUOTE_W_SPACE_DOT(getThreadID());

        res = func(m_dbWriter);
      }
      catch (...) {
        eptr = std::current_exception();
      }
    },
    Qt::ConnectionType::BlockingQueuedConnection);

  if (eptr) {
    std::rethrow_exception(eptr);
  }

  return res;
}

template <typename T>
inline T DatabaseWorker::read(const std::function<T(const QSqlDatabase&)>& func) {
  std::exception_ptr eptr = nullptr;
  std::optional<T> result;

  QFuture<void> future = QtConcurrent::run(&m_readThreadPool, [&]() {
    qDebugNN << LOGSEC_DB << "DB read job (with return) in thread" << NONQUOTE_W_SPACE_DOT(getThreadID());

    try {
      result.emplace(func(connectionForReading()));
    }
    catch (...) {
      eptr = std::current_exception();
    }
  });

  future.waitForFinished();

  if (eptr) {
    std::rethrow_exception(eptr);
  }

  return std::move(result.value());
}

#endif // DBWORKER_H
