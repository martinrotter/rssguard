#include "formwelcome.h"


FormWelcome::FormWelcome(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormWelcome) {
  m_ui->setupUi(this);
}

FormWelcome::~FormWelcome() {
  delete m_ui;
}
