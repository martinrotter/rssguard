// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/baselineedit.h"

#include <QKeyEvent>

BaseLineEdit::BaseLineEdit(QWidget* parent) : QLineEdit(parent) {}

void BaseLineEdit::submit(const QString& text) {
  setText(text);
  emit submitted(text);
}

void BaseLineEdit::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    emit submitted(text());

    event->accept();
  }

  QLineEdit::keyPressEvent(event);
}
