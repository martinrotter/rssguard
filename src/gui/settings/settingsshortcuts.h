#ifndef SETTINGSSHORTCUTS_H
#define SETTINGSSHORTCUTS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsshortcuts.h"


class SettingsShortcuts : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsShortcuts(QWidget *parent = 0);
    virtual ~SettingsShortcuts();

    void loadSettings();
    void saveSettings();

  private:
    Ui::SettingsShortcuts *m_ui;
};

#endif // SETTINGSSHORTCUTS_H
