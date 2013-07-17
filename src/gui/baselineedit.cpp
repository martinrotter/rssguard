#include <QKeyEvent>

#include "gui/baselineedit.h"


BaseLineEdit::BaseLineEdit(QWidget *parent) : QLineEdit(parent) {
}

BaseLineEdit::~BaseLineEdit() {
}

void BaseLineEdit::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    emit submitted(text());
    event->accept();
  }

  QLineEdit::keyPressEvent(event);
}
