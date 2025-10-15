// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formprogressworker.h"

#include "ui_formprogressworker.h"

#include <QKeyEvent>
#include <QPushButton>

FormProgressWorker::FormProgressWorker(QWidget* parent) : QDialog(parent), m_ui(new Ui::FormProgressWorker()) {
  m_ui->setupUi(this);

  setWindowFlags(Qt::WindowType::Dialog | Qt::WindowType::WindowTitleHint | Qt::WindowType::CustomizeWindowHint);

  connect(m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Cancel),
          &QPushButton::clicked,
          this,
          &FormProgressWorker::requestCancellation);
}

FormProgressWorker::~FormProgressWorker() {
  delete m_ui;
}

void FormProgressWorker::requestCancellation() {
  m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Cancel)->setEnabled(false), emit cancelRequested();
}

void FormProgressWorker::changeProgressRange(int from, int to) {
  if (to <= 0 || from < 0) {
    m_ui->m_progress->setRange(0, 0);
  }
  else {
    m_ui->m_progress->setRange(from, to);
  }
}

void FormProgressWorker::setProgress(int progress) {
  m_ui->m_progress->setValue(progress);
}

void FormProgressWorker::setLabel(const QString& label) {
  m_ui->m_lbl->setText(label);
}

void FormProgressWorker::onFinished() {
  accept();
}

void FormProgressWorker::onCanceled() {
  reject();
}

void FormProgressWorker::setCancelEnabled(bool visible) {
  m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Cancel)->blockSignals(!visible);
  m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Cancel)->setEnabled(visible);
}

void FormProgressWorker::keyPressEvent(QKeyEvent* event) {
  if (event->key() != Qt::Key::Key_Escape) {
    QDialog::keyPressEvent(event);
  }
}

int FormProgressWorker::doSingleWork(const QString& title,
                                     bool can_cancel,
                                     const std::function<void(QFutureWatcher<void>&)>& work_functor,
                                     const std::function<QString(int)>& label_functor) {
  setCancelEnabled(can_cancel);
  setWindowTitle(title);

  QFutureWatcher<void> wat_fut;
  QFuture<void> fut = QtConcurrent::run(work_functor, std::ref(wat_fut));

  setupFuture(fut, wat_fut, label_functor);
  return exec();
}

void FormProgressWorker::setupFuture(QFuture<void>& future,
                                     QFutureWatcher<void>& watcher,
                                     const std::function<QString(int)>& label_functor) {
  connect(&watcher, &QFutureWatcher<void>::progressRangeChanged, this, &FormProgressWorker::changeProgressRange);
  connect(&watcher, &QFutureWatcher<void>::progressValueChanged, this, &FormProgressWorker::setProgress);
  connect(&watcher, &QFutureWatcher<void>::progressValueChanged, this, [this, label_functor](int progress) {
    setLabel(label_functor(progress));
  });
  connect(&watcher, &QFutureWatcher<void>::finished, this, &FormProgressWorker::onFinished);
  connect(&watcher, &QFutureWatcher<void>::canceled, this, &FormProgressWorker::onCanceled);

  watcher.setFuture(future);
}
