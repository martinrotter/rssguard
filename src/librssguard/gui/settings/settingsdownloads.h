// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSDOWNLOADS_H
#define SETTINGSDOWNLOADS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsdownloads.h"

class SettingsDownloads : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsDownloads(Settings* settings, QWidget* parent = 0);
    virtual ~SettingsDownloads();

    inline QString title() const {
      return tr("Downloads");
    }

    void loadSettings();

    void saveSettings();

  private slots:
    void selectDownloadsDirectory();

  private:
    Ui::SettingsDownloads* m_ui;
};

#endif // SETTINGSDOWNLOADS_H
