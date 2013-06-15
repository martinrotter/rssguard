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

  protected slots:
    // Saves settings into global configuration.
    void saveSettings();

    // Load/save GUI settings.
    void loadInterface();
    void saveInterface();
    
  private:
    Ui::FormSettings *m_ui;
};

#endif // FORMSETTINGS_H
