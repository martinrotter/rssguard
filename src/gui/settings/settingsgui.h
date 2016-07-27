#ifndef SETTINGSGUI_H
#define SETTINGSGUI_H

#include <QWidget>

namespace Ui {
  class SettingsGui;
}

class SettingsGui : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsGui(QWidget *parent = 0);
    ~SettingsGui();

  private:
    Ui::SettingsGui *ui;
};

#endif // SETTINGSGUI_H
