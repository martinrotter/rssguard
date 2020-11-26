#ifndef STANDARDFEEDDETAILS_H
#define STANDARDFEEDDETAILS_H

#include <QWidget>

#include "ui_standardfeeddetails.h"

class StandardFeedDetails : public QWidget
{
  Q_OBJECT

  friend class FormStandardFeedDetails;

  public:
    explicit StandardFeedDetails(QWidget* parent = nullptr);
    ~StandardFeedDetails();

  private:
    Ui::StandardFeedDetails* ui;
};

#endif // STANDARDFEEDDETAILS_H
