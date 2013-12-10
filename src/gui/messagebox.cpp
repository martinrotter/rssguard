#include "gui/messagebox.h"


MessageBox::MessageBox(QWidget *parent) : QMessageBox(parent) {
}

MessageBox::~MessageBox() {
  qDebug("Destroying MessageBox instance.");
}
