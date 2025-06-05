// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSFEEDSMESSAGES_H
#define SETTINGSFEEDSMESSAGES_H

#include "gui/messagesview.h"
#include "gui/settings/settingspanel.h"

#include "ui_settingsfeedsmessages.h"

class SettingsFeedsMessages : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsFeedsMessages(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsFeedsMessages();

    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();
    virtual void loadUi();

  private slots:
    void updateDateTimeTooltip();
    void updateArticleMarkingPolicyDelay();

  private:
    void changeFont(QLabel& lbl);
    MessagesView::ArticleMarkingPolicy selectedArticleMarkingPolicy() const;

  private:
    void initializeMessageDateFormats();

    Ui::SettingsFeedsMessages* m_ui;
};

inline QString SettingsFeedsMessages::title() const {
  return tr("Feeds & articles");
}

#endif // SETTINGSFEEDSMESSAGES_H
