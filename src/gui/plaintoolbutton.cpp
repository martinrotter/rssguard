#include "gui/plaintoolbutton.h"

#include <QToolButton>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>


PlainToolButton::PlainToolButton(QWidget *parent) : QToolButton(parent) {
}

PlainToolButton::~PlainToolButton() {
}

void PlainToolButton::paintEvent(QPaintEvent *e) {
  Q_UNUSED(e)

  QPainter p(this);
  icon().paint(&p, QRect(QPoint(0, 0), size()));
}
