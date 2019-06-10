// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSSHORTCUTS_H
#define SETTINGSSHORTCUTS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsshortcuts.h"

class SettingsShortcuts : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsShortcuts(Settings* settings, QWidget* parent = 0);
    virtual ~SettingsShortcuts();

    inline QString title() const {
      return tr("Keyboard shortcuts");
    }

    void loadSettings();

    void saveSettings();

  private:
    Ui::SettingsShortcuts* m_ui;
};

#endif // SETTINGSSHORTCUTS_H
