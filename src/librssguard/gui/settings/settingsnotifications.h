// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSNOTIFICATIONS_H
#define SETTINGSNOTIFICATIONS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsnotifications.h"

class Settings;

class SettingsNotifications : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsNotifications(Settings* settings, QWidget* parent = nullptr);

    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private slots:
    void showScreenInfo(int index);

  private:
    Ui::SettingsNotifications m_ui;
};

inline QString SettingsNotifications::title() const {
  return tr("Notifications");
}

#endif // SETTINGSNOTIFICATIONS_H
