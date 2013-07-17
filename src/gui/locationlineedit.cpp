#include <QPaintEvent>

#include "gui/locationlineedit.h"


LocationLineEdit::LocationLineEdit(QWidget *parent)
  : BaseLineEdit(parent), m_progress(0), m_mouseSelectsAllText(true) {
}

LocationLineEdit::~LocationLineEdit() {
}

void LocationLineEdit::setProgress(int progress) {
  m_progress = progress;
}

void LocationLineEdit::focusOutEvent(QFocusEvent *event) {
  BaseLineEdit::focusOutEvent(event);

  // User now left text box, when he enters it again and clicks,
  // then all text should be selected.
  m_mouseSelectsAllText = true;
}

void LocationLineEdit::mousePressEvent(QMouseEvent *event) {
  if (m_mouseSelectsAllText) {
    event->ignore();
    selectAll();

    // User clicked and all text was selected.
    m_mouseSelectsAllText = false;
  }
  else {
    BaseLineEdit::mousePressEvent(event);
  }
}

void LocationLineEdit::paintEvent(QPaintEvent *event) {
  BaseLineEdit::paintEvent(event);
}
