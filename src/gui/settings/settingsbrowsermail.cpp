#include "settingsbrowsermail.h"
#include "ui_settingsbrowsermail.h"

SettingsBrowserMail::SettingsBrowserMail(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsBrowserMail)
{
  ui->setupUi(this);
}

SettingsBrowserMail::~SettingsBrowserMail()
{
  delete ui;
}
