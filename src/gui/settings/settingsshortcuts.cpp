#include "gui/settings/settingsshortcuts.h"

#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"


SettingsShortcuts::SettingsShortcuts(Settings *settings, QWidget *parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsShortcuts) {
  m_ui->setupUi(this);
}

SettingsShortcuts::~SettingsShortcuts() {
  delete m_ui;
}

void SettingsShortcuts::loadSettings() {
  onBeginLoadSettings();
  m_ui->m_shortcuts->populate(qApp->mainForm()->allActions());
  onEndLoadSettings();
}

void SettingsShortcuts::saveSettings() {
  onBeginSaveSettings();
  m_ui->m_shortcuts->updateShortcuts();
  DynamicShortcuts::save(qApp->mainForm()->allActions());
  onEndSaveSettings();
}
