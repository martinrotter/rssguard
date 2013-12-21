#ifndef FORMABOUT_H
#define FORMABOUT_H

#include "ui_formabout.h"

#include "core/defs.h"

#include <QDialog>


namespace Ui {
  class FormAbout;
}

class FormAbout : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormAbout(QWidget *parent);
    virtual ~FormAbout();

  private:
    Ui::FormAbout *m_ui;
};


#endif // FORMABOUT_H
