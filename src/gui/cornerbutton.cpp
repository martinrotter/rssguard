#include "gui/cornerbutton.h"

#include "gui/iconthemefactory.h"


CornerButton::CornerButton(QWidget *parent) : QToolButton(parent) {
  setToolTip(tr("Open new tab"));
  setAutoRaise(true);
  setIcon(IconThemeFactory::instance()->fromTheme("list-add"));
}

CornerButton::~CornerButton() {
  qDebug("Destroying CornerButton instance.");
}
