#include "settingslocalization.h"
#include "ui_settingslocalization.h"

SettingsLocalization::SettingsLocalization(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsLocalization)
{
  ui->setupUi(this);
}

SettingsLocalization::~SettingsLocalization()
{
  delete ui;
}
