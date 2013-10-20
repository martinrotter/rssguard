#include "gui/statusbar.h"


StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent) {
}

StatusBar::~StatusBar() {
  qDebug("Destroying StatusBar instance.");
}
