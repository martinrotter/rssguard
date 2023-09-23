// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnotifications.h"

#include "gui/notifications/notificationseditor.h"
#include "miscellaneous/application.h"
#include "miscellaneous/notificationfactory.h"
#include "miscellaneous/settings.h"

#include <QDir>
#include <QMetaEnum>
#include <QScreen>

SettingsNotifications::SettingsNotifications(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  m_ui.m_lblInfo
    ->setHelpText(tr("There are some built-in notification sounds. Just start typing \":\" and they will show up."),
                  true);

  connect(m_ui.m_checkEnableNotifications, &QCheckBox::toggled, this, &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_editor, &NotificationsEditor::someNotificationChanged, this, &SettingsNotifications::dirtifySettings);

  connect(m_ui.m_rbCustomNotifications, &QRadioButton::toggled, this, &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_rbCustomNotifications, &QRadioButton::toggled, this, &SettingsNotifications::requireRestart);

  connect(m_ui.m_rbNativeNotifications, &QRadioButton::toggled, this, &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_rbNativeNotifications, &QRadioButton::toggled, this, &SettingsNotifications::requireRestart);

  connect(m_ui.m_sbScreen, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_sbScreen, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsNotifications::requireRestart);

  m_ui.m_sbScreen->setMinimum(-1);
  m_ui.m_sbScreen->setMaximum(QGuiApplication::screens().size() - 1);

  connect(m_ui.m_sbScreen, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsNotifications::showScreenInfo);

  connect(m_ui.m_cbCustomNotificationsPosition,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &SettingsNotifications::dirtifySettings);
  connect(m_ui.m_cbCustomNotificationsPosition,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &SettingsNotifications::requireRestart);

  QMetaEnum enm = QMetaEnum::fromType<ToastNotificationsManager::NotificationPosition>();

  for (int i = 0; i < enm.keyCount(); i++) {
    m_ui.m_cbCustomNotificationsPosition
      ->addItem(ToastNotificationsManager::
                  textForPosition(ToastNotificationsManager::NotificationPosition(enm.value(i))),
                enm.value(i));
  }
}

void SettingsNotifications::loadSettings() {
  onBeginLoadSettings();

  // Load fancy notification settings.
  m_ui.m_checkEnableNotifications
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool());
  m_ui.m_editor->loadNotifications(qApp->notifications()->allNotifications());

  m_ui.m_rbNativeNotifications
    ->setChecked(!settings()->value(GROUP(GUI), SETTING(GUI::UseToastNotifications)).toBool());
  m_ui.m_sbScreen->setValue(settings()->value(GROUP(GUI), SETTING(GUI::ToastNotificationsScreen)).toInt());

  m_ui.m_cbCustomNotificationsPosition
    ->setCurrentIndex(m_ui.m_cbCustomNotificationsPosition
                        ->findData(settings()
                                     ->value(GROUP(GUI), SETTING(GUI::ToastNotificationsPosition))
                                     .value<ToastNotificationsManager::NotificationPosition>()));

  onEndLoadSettings();
}

void SettingsNotifications::saveSettings() {
  onBeginSaveSettings();

  // Save notifications.
  settings()->setValue(GROUP(GUI), GUI::EnableNotifications, m_ui.m_checkEnableNotifications->isChecked());
  qApp->notifications()->save(m_ui.m_editor->allNotifications(), settings());

  settings()->setValue(GROUP(GUI), GUI::UseToastNotifications, m_ui.m_rbCustomNotifications->isChecked());
  settings()->setValue(GROUP(GUI), GUI::ToastNotificationsScreen, m_ui.m_sbScreen->value());

  settings()->setValue(GROUP(GUI),
                       GUI::ToastNotificationsPosition,
                       m_ui.m_cbCustomNotificationsPosition->currentData()
                         .value<ToastNotificationsManager::NotificationPosition>());

  onEndSaveSettings();
}

void SettingsNotifications::showScreenInfo(int index) {
  QScreen* scr;

  if (index < 0 || index >= QGuiApplication::screens().size()) {
    scr = QGuiApplication::primaryScreen();
  }
  else {
    scr = QGuiApplication::screens().at(index);
  }

  m_ui.m_lblScreenInfo->setText(QSL("%1 (%2x%3)")
                                  .arg(scr->name(),
                                       QString::number(scr->virtualSize().width()),
                                       QString::number(scr->virtualSize().height())));
}
