#ifndef SETTINGSGUI_H
#define SETTINGSGUI_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsgui.h"


class SettingsGui : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsGui(Settings *settings, QWidget *parent = 0);
    virtual ~SettingsGui();

    void loadSettings();
    void saveSettings();

  private slots:
    void onSkinSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);

  private:
    Ui::SettingsGui *m_ui;
};

#endif // SETTINGSGUI_H
