#include "formmain.h"


FormMain::FormMain(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);
}

FormMain::~FormMain() {
  delete m_ui;
}

void FormMain::processExecutionMessage(const QString &message) {
  // TODO: Implement proper reaction when application is launched more than once.
}
