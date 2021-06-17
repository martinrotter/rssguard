// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnotifications.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/guiutilities.h"
#include "gui/notifications/notificationseditor.h"
#include "miscellaneous/application.h"
#include "miscellaneous/notificationfactory.h"
#include "miscellaneous/settings.h"

#include <QDir>

SettingsNotifications::SettingsNotifications(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  GuiUtilities::setLabelAsNotice(*m_ui.m_lblAvailableSounds, false);
  GuiUtilities::setLabelAsNotice(*m_ui.m_lblInfo, true);

  connect(m_ui.m_checkEnableNotifications, &QCheckBox::toggled, this, &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_editor, &NotificationsEditor::someNotificationChanged, this, &SettingsNotifications::dirtifySettings);
}

void SettingsNotifications::loadSettings() {
  onBeginLoadSettings();

  auto builtin_sounds = QDir(SOUNDS_BUILTIN_DIRECTORY).entryInfoList(QDir::Filter::Files,
                                                                     QDir::SortFlag::Name);
  auto iter = boolinq::from(builtin_sounds).select([](const QFileInfo& i) {
    return QSL("  %1").arg(i.absoluteFilePath());
  }).toStdList();
  auto descs = FROM_STD_LIST(QStringList, iter).join(QSL("\n"));

  m_ui.m_lblAvailableSounds->setText(QSL("Built-in sounds:\n%1").arg(descs));

  // Load fancy notification settings.
  m_ui.m_checkEnableNotifications->setChecked(settings()->value(GROUP(Notifications), SETTING(Notifications::EnableNotifications)).toBool());
  m_ui.m_editor->loadNotifications(qApp->notifications()->allNotifications());

  onEndLoadSettings();
}

void SettingsNotifications::saveSettings() {
  onBeginSaveSettings();

  // Save notifications.
  settings()->setValue(GROUP(Notifications), Notifications::EnableNotifications, m_ui.m_checkEnableNotifications->isChecked());
  qApp->notifications()->save(m_ui.m_editor->allNotifications(), settings());

  onEndSaveSettings();
}
