#include "gui/cornerbutton.h"


CornerButton::CornerButton(QWidget *parent) : QPushButton(parent) {
  setToolTip(tr("Open new tab"));
}

CornerButton::~CornerButton() {
  qDebug("Destroying CornerButton instance.");
}
