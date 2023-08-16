// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnotifications.h"

#include "gui/notifications/notificationseditor.h"
#include "miscellaneous/application.h"
#include "miscellaneous/notificationfactory.h"
#include "miscellaneous/settings.h"

#include <QDir>

SettingsNotifications::SettingsNotifications(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  m_ui.m_lblInfo
    ->setHelpText(tr("You must have \"tray icon\" activated to have balloon notifications working.\n\n"
                     "Also, there are some built-in sounds. Just start typing \":\" and they will show up."),
                  true);

  connect(m_ui.m_checkEnableNotifications, &QCheckBox::toggled, this, &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_editor, &NotificationsEditor::someNotificationChanged, this, &SettingsNotifications::dirtifySettings);
}

void SettingsNotifications::loadSettings() {
  onBeginLoadSettings();

  // Load fancy notification settings.
  m_ui.m_checkEnableNotifications
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool());
  m_ui.m_editor->loadNotifications(qApp->notifications()->allNotifications());

  onEndLoadSettings();
}

void SettingsNotifications::saveSettings() {
  onBeginSaveSettings();

  // Save notifications.
  settings()->setValue(GROUP(GUI), GUI::EnableNotifications, m_ui.m_checkEnableNotifications->isChecked());
  qApp->notifications()->save(m_ui.m_editor->allNotifications(), settings());

  onEndSaveSettings();
}
