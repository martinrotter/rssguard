// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/settings/settingspanel.h"

#include "miscellaneous/settings.h"

SettingsPanel::SettingsPanel(Settings* settings, QWidget* parent)
  : QWidget(parent), m_requiresRestart(false), m_isDirty(false), m_isLoading(false), m_settings(settings) {}

void SettingsPanel::onBeginLoadSettings() {
  m_isLoading = true;
}

void SettingsPanel::onEndLoadSettings() {
  m_isLoading = false;
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

bool SettingsPanel::isDirty() const {
  return m_isDirty;
}

void SettingsPanel::setIsDirty(bool is_dirty) {
  m_isDirty = is_dirty;
}

Settings* SettingsPanel::settings() const {
  return m_settings;
}
