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
  QPainter p(this);
  icon().paint(&p, QRect(QPoint(0, 0), iconSize()));

  /*
  if (underMouse()) {
    QStyleOptionFrameV3 style_option;
    int frame_shape = QFrame::Sunken & QFrame::Shape_Mask;

    style_option.init(this);
    style_option.frameShape = QFrame::Shape(int(style_option.frameShape) |
                                            QFrame::StyledPanel |
                                            frame_shape);
    style_option.rect = rect();
    style_option.lineWidth = 1;
    style_option.midLineWidth = 0;

    style()->drawControl(QStyle::CE_ShapedFrame,
                         &style_option, &p,
                         this);
  }
  */
}
