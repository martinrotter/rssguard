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

#include "gui/settings/settingsshortcuts.h"

#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "miscellaneous/application.h"

SettingsShortcuts::SettingsShortcuts(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsShortcuts) {
  m_ui->setupUi(this);
  connect(m_ui->m_shortcuts, &DynamicShortcutsWidget::setupChanged, this, &SettingsShortcuts::dirtifySettings);
}

SettingsShortcuts::~SettingsShortcuts() {
  delete m_ui;
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
