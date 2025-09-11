// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSGUI_H
#define SETTINGSGUI_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsgui.h"

class SettingsGui : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsGui(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsGui();

    virtual void loadUi();
    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private slots:
    void updateSkinOptions();
    void resetCustomSkinColor();

  private:
    void changeFont(QLabel& lbl);

  private:
    Ui::SettingsGui* m_ui;
};

inline QString SettingsGui::title() const {
  return tr("User interface");
}

#endif // SETTINGSGUI_H
