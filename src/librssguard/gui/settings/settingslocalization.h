// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSLOCALIZATION_H
#define SETTINGSLOCALIZATION_H

#include "gui/settings/settingspanel.h"

#include "ui_settingslocalization.h"

class SettingsLocalization : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsLocalization(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsLocalization();

    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private:
    Ui::SettingsLocalization* m_ui;
};

inline QString SettingsLocalization::title() const {
  return tr("Localization");
}

#endif // SETTINGSLOCALIZATION_H
