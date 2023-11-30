// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSDOWNLOADS_H
#define SETTINGSDOWNLOADS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsdownloads.h"

class SettingsDownloads : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsDownloads(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsDownloads();

    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private slots:
    void selectDownloadsDirectory();

  private:
    Ui::SettingsDownloads* m_ui;
};

inline QString SettingsDownloads::title() const {
  return tr("Downloads");
}

#endif // SETTINGSDOWNLOADS_H
