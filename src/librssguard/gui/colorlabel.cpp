// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/colorlabel.h"

#include <QPainter>
#include <QPaintEvent>

ColorLabel::ColorLabel(QWidget* parent) : QLabel(parent), m_color(QColor()) {
  setFixedWidth(20);
}

ColorLabel::~ColorLabel() {}

QColor ColorLabel::color() const {
  return m_color;
}

void ColorLabel::setColor(const QColor& color) {
  m_color = color;
  repaint();
}

void ColorLabel::paintEvent(QPaintEvent* event) {
  QPainter(this).fillRect(event->rect(), m_color);
}
