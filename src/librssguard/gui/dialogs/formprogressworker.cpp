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
  m_ui->m_progress->setMinimum(from);
  m_ui->m_progress->setMaximum(to);
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

void FormProgressWorker::setCancelVisible(bool visible) {
  m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Cancel)->blockSignals(!visible);
  m_ui->m_btnBox->button(QDialogButtonBox::StandardButton::Cancel)->setVisible(visible);
}

void FormProgressWorker::keyPressEvent(QKeyEvent* event) {
  if (event->key() != Qt::Key::Key_Escape) {
    QDialog::keyPressEvent(event);
  }
}

WorkerReporter::WorkerReporter(QObject* parent) : QObject(parent) {}

int FormProgressWorker::doSingleWork(const QString& title,
                                     bool can_cancel,
                                     std::function<void(WorkerReporter&)> work_functor,
                                     std::function<QString(int)> label_functor) {
  setCancelVisible(can_cancel);
  setWindowTitle(title);

  WorkerReporter report;
  QFutureWatcher<void> wat_fut;
  QFuture<void> fut = QtConcurrent::run(work_functor, std::ref(report));

  connect(&report, &WorkerReporter::progressRangeChanged, &wat_fut, &QFutureWatcher<void>::progressRangeChanged);
  connect(&report, &WorkerReporter::progressChanged, &wat_fut, &QFutureWatcher<void>::progressValueChanged);

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
