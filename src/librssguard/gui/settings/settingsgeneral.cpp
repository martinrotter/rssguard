// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsgeneral.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/systemfactory.h"

#include "ui_settingsgeneral.h"

SettingsGeneral::SettingsGeneral(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(nullptr) {}

void SettingsGeneral::loadUi() {
  m_ui = new Ui::SettingsGeneral();
  m_ui->setupUi(this);
  m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text().arg(QSL(APP_NAME)));
  m_ui->m_checkForUpdatesOnStart->setText(m_ui->m_checkForUpdatesOnStart->text().arg(QSL(APP_NAME)));

#if defined(NO_UPDATE_CHECK)
  m_ui->m_checkForUpdatesOnStart->setVisible(false);
#endif

  connect(m_ui->m_checkDisableDebugOutput, &QCheckBox::STATE_CHANGED, this, &SettingsGeneral::dirtifySettings);
  connect(m_ui->m_checkAutostart, &QCheckBox::STATE_CHANGED, this, &SettingsGeneral::dirtifySettings);
  connect(m_ui->m_checkForUpdatesOnStart, &QCheckBox::STATE_CHANGED, this, &SettingsGeneral::dirtifySettings);

  SettingsPanel::loadUi();
}

SettingsGeneral::~SettingsGeneral() {
  if (m_ui != nullptr) {
    delete m_ui;
  }
}

QIcon SettingsGeneral::icon() const {
  return qApp->icons()->fromTheme(QSL("applications-system"), QSL("emblem-system"));
}

void SettingsGeneral::loadSettings() {
  onBeginLoadSettings();

  m_ui->m_checkDisableDebugOutput
    ->setChecked(settings()->value(GROUP(General), SETTING(General::DisableDebugOutput)).toBool());

  m_ui->m_checkForUpdatesOnStart
    ->setChecked(settings()->value(GROUP(General), SETTING(General::UpdateOnStartup)).toBool());

  // Load auto-start status.
  const SystemFactory::AutoStartStatus autostart_status = qApp->system()->autoStartStatus();

  switch (autostart_status) {
    case SystemFactory::AutoStartStatus::Enabled:
      m_ui->m_checkAutostart->setChecked(true);
      break;

    case SystemFactory::AutoStartStatus::Disabled:
      m_ui->m_checkAutostart->setChecked(false);
      break;

    default:
      m_ui->m_checkAutostart->setEnabled(false);
      m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text() + tr(" (not supported on this platform)"));
      break;
  }

  onEndLoadSettings();
}

void SettingsGeneral::saveSettings() {
  onBeginSaveSettings();

  // If auto-start feature is available and user wants to turn it on, then turn it on.
  if (m_ui->m_checkAutostart->isChecked()) {
    qApp->system()->setAutoStartStatus(SystemFactory::AutoStartStatus::Enabled);
  }
  else {
    qApp->system()->setAutoStartStatus(SystemFactory::AutoStartStatus::Disabled);
  }

  settings()->setValue(GROUP(General), General::DisableDebugOutput, m_ui->m_checkDisableDebugOutput->isChecked());
  settings()->setValue(GROUP(General), General::UpdateOnStartup, m_ui->m_checkForUpdatesOnStart->isChecked());
  onEndSaveSettings();

  qApp->updateCliDebugStatus();
}
