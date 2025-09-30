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

    template <class TData>
    int doWork(const QString& title,
               bool can_cancel,
               const QList<TData>& input,
               std::function<void(TData)> work_functor,
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
    void setCancelVisible(bool visible);

  private:
    Ui::FormProgressWorker* m_ui;
};

template <class TData>
inline int FormProgressWorker::doWork(const QString& title,
                                      bool can_cancel,
                                      const QList<TData>& input,
                                      std::function<void(TData)> work_functor,
                                      std::function<QString(int)> label_functor) {
  setCancelVisible(can_cancel);
  setWindowTitle(title);

  QFuture<void> fut = QtConcurrent::map(input, work_functor);
  QFutureWatcher<void> wat_fut;

  connect(&wat_fut, &QFutureWatcher<void>::progressRangeChanged, this, &FormProgressWorker::changeProgressRange);
  connect(&wat_fut, &QFutureWatcher<void>::progressValueChanged, this, &FormProgressWorker::setProgress);
  connect(&wat_fut, &QFutureWatcher<void>::progressValueChanged, this, [this, label_functor](int progress) {
    setLabel(label_functor(progress));
  });
  connect(&wat_fut, &QFutureWatcher<void>::finished, this, &FormProgressWorker::onFinished);
  connect(&wat_fut, &QFutureWatcher<void>::canceled, this, &FormProgressWorker::onCanceled);

  wat_fut.setFuture(fut);
  return exec();
}

#endif // FORMPROGRESSWORKER_H
