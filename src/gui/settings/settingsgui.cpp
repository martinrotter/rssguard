#include "settingsgui.h"
#include "ui_settingsgui.h"

SettingsGui::SettingsGui(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsGui)
{
  ui->setupUi(this);
}

SettingsGui::~SettingsGui()
{
  delete ui;
}
