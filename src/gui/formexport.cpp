#include "formexport.h"
#include "ui_formexport.h"

FormExport::FormExport(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::FormExport)
{
  ui->setupUi(this);
}

FormExport::~FormExport()
{
  delete ui;
}
