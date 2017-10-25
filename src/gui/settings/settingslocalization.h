// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSLOCALIZATION_H
#define SETTINGSLOCALIZATION_H

#include "gui/settings/settingspanel.h"

#include "ui_settingslocalization.h"

class SettingsLocalization : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsLocalization(Settings* settings, QWidget* parent = 0);
    virtual ~SettingsLocalization();

    inline QString title() const {
      return tr("Language");
    }

    void loadSettings();

    void saveSettings();

  private:
    Ui::SettingsLocalization* m_ui;
};

#endif // SETTINGSLOCALIZATION_H
