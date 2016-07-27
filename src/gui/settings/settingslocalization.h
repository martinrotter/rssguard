#ifndef SETTINGSLOCALIZATION_H
#define SETTINGSLOCALIZATION_H

#include "gui/settings/settingspanel.h"

#include "ui_settingslocalization.h"


class SettingsLocalization : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsLocalization(Settings *settings, QWidget *parent = 0);
    virtual ~SettingsLocalization();

    void loadSettings();
    void saveSettings();

  private:
    Ui::SettingsLocalization *m_ui;
};

#endif // SETTINGSLOCALIZATION_H
