// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsshortcuts.h"

#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

SettingsShortcuts::SettingsShortcuts(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(nullptr) {}

SettingsShortcuts::~SettingsShortcuts() {
  if (m_ui != nullptr) {
    delete m_ui;
  }
}

void SettingsShortcuts::loadUi() {
  m_ui = new Ui::SettingsShortcuts();
  m_ui->setupUi(this);
  connect(m_ui->m_shortcuts, &DynamicShortcutsWidget::setupChanged, this, &SettingsShortcuts::dirtifySettings);

  SettingsPanel::loadUi();
}

QIcon SettingsShortcuts::icon() const {
  return qApp->icons()->fromTheme(QSL("configure-shortcuts"), QSL("keyboard"));
}

void SettingsShortcuts::loadSettings() {
  onBeginLoadSettings();
  m_ui->m_shortcuts->populate(qApp->userAndExtraActions());
  onEndLoadSettings();
}

void SettingsShortcuts::saveSettings() {
  onBeginSaveSettings();
  m_ui->m_shortcuts->updateShortcuts();
  DynamicShortcuts::save(qApp->userAndExtraActions());
  onEndSaveSettings();
}
