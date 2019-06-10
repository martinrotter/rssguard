// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSGUI_H
#define SETTINGSGUI_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsgui.h"

class SettingsGui : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsGui(Settings* settings, QWidget* parent = 0);
    virtual ~SettingsGui();

    inline QString title() const {
      return tr("User interface");
    }

    void loadSettings();

    void saveSettings();

  protected:

    // Does check of controls before dialog can be submitted.
    bool eventFilter(QObject* obj, QEvent* e);

  private:
    Ui::SettingsGui* m_ui;
};

#endif // SETTINGSGUI_H
