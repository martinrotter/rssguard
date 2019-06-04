// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSFEEDSMESSAGES_H
#define SETTINGSFEEDSMESSAGES_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsfeedsmessages.h"

class SettingsFeedsMessages : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsFeedsMessages(Settings* settings, QWidget* parent = 0);
    virtual ~SettingsFeedsMessages();

    inline QString title() const {
      return tr("Feeds & messages");
    }

    void loadSettings();

    void saveSettings();

  private:
    void changeFont(QLabel& lbl);

  private:
    void initializeMessageDateFormats();

    Ui::SettingsFeedsMessages* m_ui;
};

#endif // SETTINGSFEEDSMESSAGES_H
