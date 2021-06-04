// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnotifications.h"

#include "miscellaneous/settings.h"

SettingsNotifications::SettingsNotifications(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  connect(m_ui.m_checkEnableNotifications, &QCheckBox::toggled, this, &SettingsNotifications::dirtifySettings);
}

void SettingsNotifications::loadSettings() {
  onBeginLoadSettings();

  // Load fancy notification settings.
  m_ui.m_checkEnableNotifications->setChecked(settings()->value(GROUP(Notifications), SETTING(Notifications::EnableNotifications)).toBool());

  onEndLoadSettings();
}

void SettingsNotifications::saveSettings() {
  onBeginSaveSettings();

  // Save notifications.
  settings()->setValue(GROUP(Notifications), Notifications::EnableNotifications, m_ui.m_checkEnableNotifications->isChecked());

  onEndSaveSettings();
}
