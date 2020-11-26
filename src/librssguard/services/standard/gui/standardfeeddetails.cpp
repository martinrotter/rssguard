#include "standardfeeddetails.h"

StandardFeedDetails::StandardFeedDetails(QWidget* parent) :
  QWidget(parent),
  ui(new Ui::StandardFeedDetails)
{
  ui->setupUi(this);
}

StandardFeedDetails::~StandardFeedDetails()
{
  delete ui;
}
