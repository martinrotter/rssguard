#ifndef SETTINGSGENERAL_H
#define SETTINGSGENERAL_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsgeneral.h"


class SettingsGeneral : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsGeneral(Settings *settings, QWidget *parent = 0);
    virtual ~SettingsGeneral();

  protected:
    void loadSettings();
    void saveSettings();

  private:
    Ui::SettingsGeneral *m_ui;
};

#endif // SETTINGSGENERAL_H
