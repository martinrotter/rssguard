#ifndef SETTINGSDOWNLOADS_H
#define SETTINGSDOWNLOADS_H

#include <QWidget>

namespace Ui {
  class SettingsDownloads;
}

class SettingsDownloads : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsDownloads(QWidget *parent = 0);
    ~SettingsDownloads();

  private:
    Ui::SettingsDownloads *ui;
};

#endif // SETTINGSDOWNLOADS_H
