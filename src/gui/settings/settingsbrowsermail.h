#ifndef SETTINGSBROWSERMAIL_H
#define SETTINGSBROWSERMAIL_H

#include <QWidget>

namespace Ui {
  class SettingsBrowserMail;
}

class SettingsBrowserMail : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsBrowserMail(QWidget *parent = 0);
    ~SettingsBrowserMail();

  private:
    Ui::SettingsBrowserMail *ui;
};

#endif // SETTINGSBROWSERMAIL_H
