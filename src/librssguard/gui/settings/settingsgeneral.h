// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSGENERAL_H
#define SETTINGSGENERAL_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsgeneral.h"

class SettingsGeneral : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsGeneral(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsGeneral();

    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();
    virtual void loadUi();

  private:
    Ui::SettingsGeneral* m_ui;
};

inline QString SettingsGeneral::title() const {
  return tr("General");
}

#endif // SETTINGSGENERAL_H
