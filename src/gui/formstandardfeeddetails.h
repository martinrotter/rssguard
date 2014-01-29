#ifndef FORMSTANDARDFEEDDETAILS_H
#define FORMSTANDARDFEEDDETAILS_H

#include <QDialog>

#include "ui_formstandardfeeddetails.h"


namespace Ui {
  class FormStandardFeedDetails;
}

class FeedsModel;

class FormStandardFeedDetails : public QDialog {
    Q_OBJECT

  public:
    explicit FormStandardFeedDetails(FeedsModel *model, QWidget *parent = 0);
    virtual ~FormStandardFeedDetails();

  private:
    Ui::FormStandardFeedDetails *m_ui;
};

#endif // FORMSTANDARDFEEDDETAILS_H
