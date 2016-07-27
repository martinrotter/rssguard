#ifndef SETTINGSLOCALIZATION_H
#define SETTINGSLOCALIZATION_H

#include <QWidget>

namespace Ui {
  class SettingsLocalization;
}

class SettingsLocalization : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsLocalization(QWidget *parent = 0);
    ~SettingsLocalization();

  private:
    Ui::SettingsLocalization *ui;
};

#endif // SETTINGSLOCALIZATION_H
