#include "gui/formstandardfeeddetails.h"

#include "core/feedsmodel.h"


FormStandardFeedDetails::FormStandardFeedDetails(FeedsModel *model, QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormStandardFeedDetails) {
  m_ui->setupUi(this);
}

FormStandardFeedDetails::~FormStandardFeedDetails() {
  delete m_ui;
}
