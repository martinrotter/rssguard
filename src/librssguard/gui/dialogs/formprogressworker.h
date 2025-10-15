// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMPROGRESSWORKER_H
#define FORMPROGRESSWORKER_H

#include <QDialog>
#include <QFutureWatcher>
#include <QtConcurrentMap>
#include <QtConcurrentRun>

namespace Ui {
  class FormProgressWorker;
}

class RSSGUARD_DLLSPEC FormProgressWorker : public QDialog {
    Q_OBJECT

  public:
    explicit FormProgressWorker(QWidget* parent = nullptr);
    virtual ~FormProgressWorker();

    int doSingleWork(const QString& title,
                     bool can_cancel,
                     const std::function<void(QFutureWatcher<void>&)>& work_functor,
                     const std::function<QString(int)>& label_functor);

    template <class TInput>
    int doWork(const QString& title,
               bool can_cancel,
               const QList<TInput>& input,
               std::function<void(TInput)> work_functor,
               std::function<QString(int)> label_functor);

  protected:
    virtual void keyPressEvent(QKeyEvent* event) override;

  signals:
    void cancelRequested();

  private slots:
    void requestCancellation();
    void changeProgressRange(int from, int to);
    void setProgress(int progress);
    void setLabel(const QString& label);
    void onFinished();
    void onCanceled();

  private:
    void setupFuture(QFuture<void>& future,
                     QFutureWatcher<void>& watcher,
                     const std::function<QString(int)>& label_functor);
    void setCancelEnabled(bool visible);

  private:
    Ui::FormProgressWorker* m_ui;
};

template <class TInput>
inline int FormProgressWorker::doWork(const QString& title,
                                      bool can_cancel,
                                      const QList<TInput>& input,
                                      std::function<void(TInput)> work_functor,
                                      std::function<QString(int)> label_functor) {
  setCancelEnabled(can_cancel);
  setWindowTitle(title);

  QFuture<void> fut = QtConcurrent::map(input, work_functor);
  QFutureWatcher<void> wat_fut;

  setupFuture(fut, wat_fut, label_functor);
  return exec();
}

#endif // FORMPROGRESSWORKER_H
