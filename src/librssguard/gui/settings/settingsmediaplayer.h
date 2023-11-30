// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSMEDIAPLAYER_H
#define SETTINGSMEDIAPLAYER_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsmediaplayer.h"

class SettingsMediaPlayer : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsMediaPlayer(Settings* settings, QWidget* parent = nullptr);

    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private:
    Ui::SettingsMediaPlayer m_ui;
};

inline QString SettingsMediaPlayer::title() const {
  return tr("Media player");
}

#endif // SETTINGSMEDIAPLAYER_H
