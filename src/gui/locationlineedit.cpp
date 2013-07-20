#include <QPaintEvent>
#include <QStyleOptionFrameV2>
#include <QPainter>
#include <QApplication>

#include "gui/locationlineedit.h"


LocationLineEdit::LocationLineEdit(QWidget *parent)
  : BaseLineEdit(parent),
    m_progress(0),
    m_defaultPalette(palette()),
    m_mouseSelectsAllText(true) {
}

LocationLineEdit::~LocationLineEdit() {
}

void LocationLineEdit::setProgress(int progress) {
  m_progress = progress;
  update();
}

void LocationLineEdit::clearProgress() {
  setProgress(0);
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
  // Draw "progress bar" if needed.
  if (m_progress > 0) {
    QPalette current_palette = palette();
    QColor loadingColor = QColor(0, 255, 0, 100);
    QLinearGradient gradient(0, 0, width(), 0);
    qreal percentage_border = m_progress / 100.0;

    gradient.setColorAt(0, loadingColor);
    gradient.setColorAt(percentage_border - 0.01, loadingColor);
    gradient.setColorAt(percentage_border - 0.008, loadingColor.lighter(130));
    //gradient.setColorAt(percentage_border - 0.002, loadingColor);
    gradient.setColorAt(percentage_border, QApplication::palette().color(QPalette::Base));
    current_palette.setBrush(QPalette::Base, gradient);

    setPalette(current_palette);
  }
  // No "progress bar" is needed, restore default palette.
  else {
    setPalette(m_defaultPalette);
  }

  BaseLineEdit::paintEvent(event);
}
