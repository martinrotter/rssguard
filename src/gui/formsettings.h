#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include <QDialog>

#include "ui_formsettings.h"


namespace Ui {
  class FormSettings;
}

class FormSettings : public QDialog {
    Q_OBJECT
    
  public:
    explicit FormSettings(QWidget *parent = 0);
    ~FormSettings();
    
  private:
    Ui::FormSettings *m_ui;
};

#endif // FORMSETTINGS_H
