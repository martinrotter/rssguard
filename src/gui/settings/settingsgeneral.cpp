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

#include "gui/settings/settingsgeneral.h"

#include "miscellaneous/systemfactory.h"
#include "miscellaneous/application.h"


SettingsGeneral::SettingsGeneral(Settings* settings, QWidget* parent)
	: SettingsPanel(settings, parent), m_ui(new Ui::SettingsGeneral) {
	m_ui->setupUi(this);
	m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text().arg(APP_NAME));
	connect(m_ui->m_checkAutostart, &QCheckBox::stateChanged, this, &SettingsGeneral::dirtifySettings);
	connect(m_ui->m_checkForUpdatesOnStart, &QCheckBox::stateChanged, this, &SettingsGeneral::dirtifySettings);
	connect(m_ui->m_checkRemoveTrolltechJunk, &QCheckBox::stateChanged, this, &SettingsGeneral::dirtifySettings);
}

SettingsGeneral::~SettingsGeneral() {
	delete m_ui;
}

void SettingsGeneral::loadSettings() {
	onBeginLoadSettings();
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
	onEndLoadSettings();
}

void SettingsGeneral::saveSettings() {
	onBeginSaveSettings();

	// If auto-start feature is available and user wants to turn it on, then turn it on.
	if (m_ui->m_checkAutostart->isChecked()) {
		qApp->system()->setAutoStartStatus(SystemFactory::Enabled);
	}

	else {
		qApp->system()->setAutoStartStatus(SystemFactory::Disabled);
	}

	settings()->setValue(GROUP(General), General::UpdateOnStartup, m_ui->m_checkForUpdatesOnStart->isChecked());
	settings()->setValue(GROUP(General), General::RemoveTrolltechJunk, m_ui->m_checkRemoveTrolltechJunk->isChecked());
	onEndSaveSettings();
}
