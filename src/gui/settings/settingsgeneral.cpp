#include "gui/settings/settingsgeneral.h"

#include "miscellaneous/systemfactory.h"
#include "miscellaneous/application.h"


SettingsGeneral::SettingsGeneral(Settings *settings, QWidget *parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsGeneral) {
  m_ui->setupUi(this);
  m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text().arg(APP_NAME));
}

SettingsGeneral::~SettingsGeneral() {
  delete m_ui;
}

void SettingsGeneral::loadSettings() {
  m_ui->m_checkForUpdatesOnStart->setChecked(settings()->value(GROUP(General), SETTING(General::UpdateOnStartup)).toBool());

  // Load auto-start status.
  const SystemFactory::AutoStartStatus autostart_status = qApp->system()->getAutoStartStatus();

  switch (autostart_status) {
    case SystemFactory::Enabled:
      m_ui->m_checkAutostart->setChecked(true);
      break;

    case SystemFactory::Disabled:
      m_ui->m_checkAutostart->setChecked(false);
      break;

    default:
      m_ui->m_checkAutostart->setEnabled(false);
      m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text() + tr(" (not supported on this platform)"));
      break;
  }

#if defined(Q_OS_WIN)
  m_ui->m_checkRemoveTrolltechJunk->setVisible(true);
  m_ui->m_checkRemoveTrolltechJunk->setChecked(settings()->value(GROUP(General), SETTING(General::RemoveTrolltechJunk)).toBool());
#else
  m_ui->m_checkRemoveTrolltechJunk->setVisible(false);
#endif

  SettingsPanel::loadSettings();
}

void SettingsGeneral::saveSettings() {
  // If auto-start feature is available and user wants to turn it on, then turn it on.
  if (m_ui->m_checkAutostart->isChecked()) {
    qApp->system()->setAutoStartStatus(SystemFactory::Enabled);
  }
  else {
    qApp->system()->setAutoStartStatus(SystemFactory::Disabled);
  }

  settings()->setValue(GROUP(General), General::UpdateOnStartup, m_ui->m_checkForUpdatesOnStart->isChecked());
  settings()->setValue(GROUP(General), General::RemoveTrolltechJunk, m_ui->m_checkRemoveTrolltechJunk->isChecked());

  SettingsPanel::saveSettings();
}
