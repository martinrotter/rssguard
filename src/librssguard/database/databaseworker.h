// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DBWORKER_H
#define DBWORKER_H

#include <functional>

#include <QSqlDatabase>
#include <QThreadPool>

class RSSGUARD_DLLSPEC DatabaseWorker : public QObject {
    Q_OBJECT

  public:
    explicit DatabaseWorker(QObject* parent = nullptr);
    virtual ~DatabaseWorker();

    void read(const std::function<void(QSqlDatabase)>& func);
    void write(const std::function<void(QSqlDatabase)>& func);

  signals:
    void executeWrite(std::function<void(QSqlDatabase)> func);

  private slots:
    void onExecuteWrite(const std::function<void(QSqlDatabase)>& func);

  private:
    QThreadPool m_readThreadPool;
    QThread m_writeThread;
    QSqlDatabase m_dbWriter;
};

#endif // DBWORKER_H
