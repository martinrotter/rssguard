#ifndef FORMWELCOME_H
#define FORMWELCOME_H

#include <QDialog>

#include "ui_formwelcome.h"


namespace Ui {
  class FormWelcome;
}

class FormWelcome : public QDialog {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit FormWelcome(QWidget *parent = 0);
    ~FormWelcome();
    
  private:
    Ui::FormWelcome *m_ui;
};

#endif // FORMWELCOME_H
