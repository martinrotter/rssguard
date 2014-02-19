#include "formupdate.h"


FormUpdate::FormUpdate(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormUpdate) {
  m_ui->setupUi(this);
}

FormUpdate::~FormUpdate()
{
  delete m_ui;
}
