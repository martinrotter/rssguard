// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSTOOLBARS_H
#define SETTINGSTOOLBARS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingstoolbars.h"

class SettingsToolbars : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsToolbars(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsToolbars();

    virtual void loadUi();
    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  protected:
    // Does check of controls before dialog can be submitted.
    bool eventFilter(QObject* obj, QEvent* e);

  private:
    void changeFont(QLabel& lbl);

  private:
    Ui::SettingsToolbars* m_ui;
};

inline QString SettingsToolbars::title() const {
  return tr("Toolbars");
}

#endif // SETTINGSTOOLBARS_H
