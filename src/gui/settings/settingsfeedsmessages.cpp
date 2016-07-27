#include "settingsfeedsmessages.h"
#include "ui_settingsfeedsmessages.h"

SettingsFeedsMessages::SettingsFeedsMessages(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsFeedsMessages)
{
  ui->setupUi(this);
}

SettingsFeedsMessages::~SettingsFeedsMessages()
{
  delete ui;
}
