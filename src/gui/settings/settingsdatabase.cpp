#include "gui/settings/settingsdatabase.h"


SettingsDatabase::SettingsDatabase(QWidget *parent)
  : QWidget(parent), m_ui(new Ui::SettingsDatabase) {
  m_ui->setupUi(this);
}

SettingsDatabase::~SettingsDatabase() {
  delete m_ui;
}

void SettingsDatabase::loadSettings() {
  onBeginLoadSettings();


  onEndLoadSettings();
}

void SettingsDatabase::saveSettings() {
  onBeginSaveSettings();


  onEndSaveSettings();
}
