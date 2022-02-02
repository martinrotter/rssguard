// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingspanel.h"

#include "miscellaneous/settings.h"

SettingsPanel::SettingsPanel(Settings* settings, QWidget* parent)
  : QWidget(parent), m_requiresRestart(false), m_isDirty(false), m_isLoading(false), m_isLoaded(false), m_settings(settings) {}

void SettingsPanel::onBeginLoadSettings() {
  m_isLoading = true;
}

void SettingsPanel::onEndLoadSettings() {
  m_isLoading = false;
  m_isLoaded = true;

  setRequiresRestart(false);
  setIsDirty(false);
}

void SettingsPanel::onBeginSaveSettings() {}

void SettingsPanel::onEndSaveSettings() {
  setIsDirty(false);
}

void SettingsPanel::dirtifySettings() {
  if (!m_isLoading) {
    setIsDirty(true);
    emit settingsChanged();
  }
}

bool SettingsPanel::requiresRestart() const {
  return m_requiresRestart;
}

void SettingsPanel::setRequiresRestart(bool requiresRestart) {
  m_requiresRestart = requiresRestart;
}

void SettingsPanel::requireRestart() {
  setRequiresRestart(true);
}

bool SettingsPanel::isLoaded() const {
  return m_isLoaded;
}

bool SettingsPanel::isDirty() const {
  return m_isDirty;
}

void SettingsPanel::setIsDirty(bool is_dirty) {
  m_isDirty = is_dirty;
}

Settings* SettingsPanel::settings() const {
  return m_settings;
}
