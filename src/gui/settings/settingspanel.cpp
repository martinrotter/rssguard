// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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


SettingsPanel::SettingsPanel(Settings *settings, QWidget *parent) : QWidget(parent), m_settings(settings) {
}

void SettingsPanel::loadSettings() {
  setIsDirty(false);
}

void SettingsPanel::saveSettings() {
  setIsDirty(false);
}

void SettingsPanel::dirtifySettings() {
  setIsDirty(true);
  emit settingsChanged();
}

bool SettingsPanel::isDirty() const {
  return m_isDirty;
}

void SettingsPanel::setIsDirty(bool is_dirty) {
  m_isDirty = is_dirty;
}

Settings *SettingsPanel::settings() const {
  return m_settings;
}
