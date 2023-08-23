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

    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  protected:

    // Does check of controls before dialog can be submitted.
    bool eventFilter(QObject* obj, QEvent* e);

  private slots:
    void updateSkinOptions();
    void resetCustomSkinColor();

  private:
    Ui::SettingsGui* m_ui;
};

inline QString SettingsGui::title() const {
  return tr("User interface");
}

#endif // SETTINGSGUI_H
