#ifndef FORMSTANDARDFEEDDETAILS_H
#define FORMSTANDARDFEEDDETAILS_H

#include <QDialog>

#include "ui_formstandardfeeddetails.h"


namespace Ui {
  class FormStandardFeedDetails;
}

class FeedsModel;
class FeedsModelStandardFeed;

class FormStandardFeedDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormStandardFeedDetails(FeedsModel *model, QWidget *parent = 0);
    virtual ~FormStandardFeedDetails();

  public slots:
    int exec(FeedsModelStandardFeed *input_feed);

  protected:
    void initialize();

  private:
    Ui::FormStandardFeedDetails *m_ui;
};

#endif // FORMSTANDARDFEEDDETAILS_H
