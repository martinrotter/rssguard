#include "settingsdownloads.h"
#include "ui_settingsdownloads.h"

SettingsDownloads::SettingsDownloads(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsDownloads)
{
  ui->setupUi(this);
}

SettingsDownloads::~SettingsDownloads()
{
  delete ui;
}
