#include "updater/formupdater.h"

#include "definitions/definitions.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>


FormUpdater::FormUpdater(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("RSS Guard updater");
  setWindowIcon(QIcon(APP_ICON_PATH));

  moveToCenterAndResize();
}

FormUpdater::~FormUpdater() {
}

void FormUpdater::moveToCenterAndResize() {
  resize(500, 400);
  move(qApp->desktop()->screenGeometry().center() - rect().center());
}
