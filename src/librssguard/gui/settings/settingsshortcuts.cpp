// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsshortcuts.h"

#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

SettingsShortcuts::SettingsShortcuts(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsShortcuts) {
  m_ui->setupUi(this);
  connect(m_ui->m_shortcuts, &DynamicShortcutsWidget::setupChanged, this, &SettingsShortcuts::dirtifySettings);
}

SettingsShortcuts::~SettingsShortcuts() {
  delete m_ui;
}

QIcon SettingsShortcuts::icon() const {
  return qApp->icons()->fromTheme(QSL("configure-shortcuts"), QSL("keyboard"));
}

void SettingsShortcuts::loadSettings() {
  onBeginLoadSettings();
  m_ui->m_shortcuts->populate(qApp->userActions());
  onEndLoadSettings();
}

void SettingsShortcuts::saveSettings() {
  onBeginSaveSettings();
  m_ui->m_shortcuts->updateShortcuts();
  DynamicShortcuts::save(qApp->userActions());
  onEndSaveSettings();
}
