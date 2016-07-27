#include "gui/settings/settingsshortcuts.h"


SettingsShortcuts::SettingsShortcuts(QWidget *parent)
  : QWidget(parent), m_ui(new Ui::SettingsShortcuts) {
  m_ui->setupUi(this);
}

SettingsShortcuts::~SettingsShortcuts() {
  delete m_ui;
}

void SettingsShortcuts::loadSettings() {
  onBeginLoadSettings();


  onEndLoadSettings();
}

void SettingsShortcuts::saveSettings() {
  onBeginSaveSettings();


  onEndSaveSettings();
}
