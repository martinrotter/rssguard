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
