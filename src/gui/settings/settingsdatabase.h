#ifndef SETTINGSDATABASE_H
#define SETTINGSDATABASE_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsdatabase.h"


class SettingsDatabase : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsDatabase(QWidget *parent = 0);
    virtual ~SettingsDatabase();

    void loadSettings();
    void saveSettings();

  private:
    Ui::SettingsDatabase *m_ui;
};

#endif // SETTINGSDATABASE_H
