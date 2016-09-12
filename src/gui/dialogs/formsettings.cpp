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

#include "gui/dialogs/formsettings.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"
#include "gui/messagebox.h"

#include "gui/settings/settingsbrowsermail.h"
#include "gui/settings/settingsdatabase.h"
#include "gui/settings/settingsdownloads.h"
#include "gui/settings/settingsfeedsmessages.h"
#include "gui/settings/settingsgeneral.h"
#include "gui/settings/settingsgui.h"
#include "gui/settings/settingslocalization.h"
#include "gui/settings/settingsshortcuts.h"


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_panels(QList<SettingsPanel*>()), m_ui(new Ui::FormSettings), m_settings(qApp->settings()) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("emblem-system")));

  m_btnApply = m_ui->m_buttonBox->button(QDialogButtonBox::Apply);
  m_btnApply->setEnabled(false);

  // Establish needed connections.
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormSettings::saveSettings);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &FormSettings::cancelSettings);
  connect(m_btnApply, &QPushButton::clicked, this, &FormSettings::applySettings);

  addSettingsPanel(new SettingsGeneral(m_settings, this));
  addSettingsPanel(new SettingsDatabase(m_settings, this));
  addSettingsPanel(new SettingsGui(m_settings, this));
  addSettingsPanel(new SettingsLocalization(m_settings, this));
  addSettingsPanel(new SettingsShortcuts(m_settings, this));
  addSettingsPanel(new SettingsBrowserMail(m_settings, this));
  addSettingsPanel(new SettingsDownloads(m_settings, this));
  addSettingsPanel(new SettingsFeedsMessages(m_settings, this));

  m_ui->m_listSettings->setCurrentRow(0);
}

FormSettings::~FormSettings() {
  qDebug("Destroying FormSettings distance.");
}

void FormSettings::saveSettings() {
  applySettings();
  accept();
}

void FormSettings::applySettings() {
  // Save all settings.
  m_settings->checkSettings();

  QStringList panels_for_restart;

  foreach (SettingsPanel *panel, m_panels) {
    if (panel->isDirty()) {
      panel->saveSettings();
    }

    if (panel->requiresRestart()) {
      panels_for_restart.append(panel->title().toLower());
      panel->setRequiresRestart(false);
    }
  }

  if (!panels_for_restart.isEmpty()) {
    const QStringList changed_settings_description = panels_for_restart.replaceInStrings(QRegExp(QSL("^")), QString::fromUtf8(" • "));

    MessageBox::show(this,
                     QMessageBox::Question,
                     tr("Critical settings were changed"),
                     tr("Some critical settings were changed and will be applied after the application gets restarted."
                        "\n\nYou have to restart manually."),
                     QString(),
                     tr("Changed categories of settings:\n%1.").arg(changed_settings_description .join(QSL(",\n"))),
                     QMessageBox::Ok, QMessageBox::Ok);
  }

  m_btnApply->setEnabled(false);
}

void FormSettings::cancelSettings() {
  QStringList changed_panels;

  foreach (SettingsPanel *panel, m_panels) {
    if (panel->isDirty()) {
      changed_panels.append(panel->title().toLower());
    }
  }

  if (changed_panels.isEmpty()) {
    reject();
  }
  else {
    const QStringList changed_settings_description = changed_panels.replaceInStrings(QRegExp(QSL("^")), QString::fromUtf8(" • "));

    if (MessageBox::show(this,
                     QMessageBox::Critical,
                     tr("Some settings are changed and will be lost"),
                     tr("Some settings were changed and by cancelling this dialog, you would lose these changes."),
                     tr("Do you really want to close this dialog without saving any settings?"),
                     tr("Changed categories of settings:\n%1.").arg(changed_settings_description .join(QSL(",\n"))),
                     QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) ==
        QMessageBox::Yes) {
      reject();
    }
  }
}

void FormSettings::addSettingsPanel(SettingsPanel *panel) {
  m_ui->m_listSettings->addItem(panel->title());
  m_panels.append(panel);
  m_ui->m_stackedSettings->addWidget(panel);
  panel->loadSettings();

  connect(panel, &SettingsPanel::settingsChanged, [this]() {
    m_btnApply->setEnabled(true);
  });
}
