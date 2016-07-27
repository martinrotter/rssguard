#ifndef SETTINGSFEEDSMESSAGES_H
#define SETTINGSFEEDSMESSAGES_H

#include <QWidget>

namespace Ui {
  class SettingsFeedsMessages;
}

class SettingsFeedsMessages : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsFeedsMessages(QWidget *parent = 0);
    ~SettingsFeedsMessages();

  private:
    Ui::SettingsFeedsMessages *ui;
};

#endif // SETTINGSFEEDSMESSAGES_H
