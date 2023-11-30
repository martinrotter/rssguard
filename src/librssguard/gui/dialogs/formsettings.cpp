// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formsettings.h"

#include "definitions/definitions.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include "gui/guiutilities.h"
#include "gui/settings/settingsbrowsermail.h"
#include "gui/settings/settingsdatabase.h"
#include "gui/settings/settingsdownloads.h"
#include "gui/settings/settingsfeedsmessages.h"
#include "gui/settings/settingsgeneral.h"
#include "gui/settings/settingsgui.h"
#include "gui/settings/settingslocalization.h"
#include "gui/settings/settingsmediaplayer.h"
#include "gui/settings/settingsnodejs.h"
#include "gui/settings/settingsnotifications.h"
#include "gui/settings/settingsshortcuts.h"

#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>

FormSettings::FormSettings(QWidget& parent) : QDialog(&parent), m_settings(*qApp->settings()) {
  m_ui.setupUi(this);

  // Set flags and attributes.
  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->fromTheme(QSL("emblem-system"), QSL("applications-system")));

  m_btnApply = m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Apply);

  m_btnApply->setEnabled(false);

  // Establish needed connections.
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormSettings::saveSettings);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormSettings::cancelSettings);
  connect(m_btnApply, &QPushButton::clicked, this, &FormSettings::applySettings);
  connect(m_ui.m_listSettings, &QListWidget::currentRowChanged, this, &FormSettings::openSettingsCategory);

  addSettingsPanel(new SettingsGeneral(&m_settings, this));
  addSettingsPanel(new SettingsDatabase(&m_settings, this));
  addSettingsPanel(new SettingsGui(&m_settings, this));
  addSettingsPanel(new SettingsNotifications(&m_settings, this));
  addSettingsPanel(new SettingsLocalization(&m_settings, this));
  addSettingsPanel(new SettingsShortcuts(&m_settings, this));
  addSettingsPanel(new SettingsBrowserMail(&m_settings, this));
  addSettingsPanel(new SettingsNodejs(&m_settings, this));
  addSettingsPanel(new SettingsMediaPlayer(&m_settings, this));
  addSettingsPanel(new SettingsDownloads(&m_settings, this));
  addSettingsPanel(new SettingsFeedsMessages(&m_settings, this));

  m_ui.m_listSettings->setMaximumWidth(m_ui.m_listSettings->sizeHintForColumn(0) +
                                       6 * m_ui.m_listSettings->frameWidth());
  m_ui.m_listSettings->setCurrentRow(0);

  resize(qApp->settings()->value(GROUP(GUI), GUI::SettingsWindowInitialSize, size()).toSize());
}

FormSettings::~FormSettings() {
  qDebugNN << LOGSEC_GUI << "Destroying FormSettings distance.";
}

void FormSettings::openSettingsCategory(int category) {
  if (category >= 0 && category < m_panels.size()) {
    if (!m_panels.at(category)->isLoaded()) {
      m_panels.at(category)->loadSettings();
    }
  }

  m_ui.m_stackedSettings->setCurrentIndex(category);
}

void FormSettings::saveSettings() {
  applySettings();
  accept();
}

void FormSettings::applySettings() {
  // Save all settings.
  m_settings.checkSettings();
  QStringList panels_for_restart;

  for (SettingsPanel* panel : std::as_const(m_panels)) {
    if (panel->isDirty() && panel->isLoaded()) {
      panel->saveSettings();
    }

    if (panel->requiresRestart()) {
      panels_for_restart.append(panel->title().toLower());
      panel->setRequiresRestart(false);
    }
  }

  if (!panels_for_restart.isEmpty()) {
    const QStringList changed_settings_description =
      panels_for_restart.replaceInStrings(QRegularExpression(QSL("^")), QString::fromUtf8(QByteArray(" • ")));
    const QMessageBox::StandardButton clicked_button =
      MsgBox::show(this,
                   QMessageBox::Icon::Question,
                   tr("Critical settings were changed"),
                   tr("Some critical settings were changed and will be applied after the application gets restarted. "
                      "\n\nYou have to restart manually."),
                   tr("Do you want to restart now?"),
                   tr("Changed categories of settings:\n%1.").arg(changed_settings_description.join(QSL(",\n"))),
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::Yes);

    if (clicked_button == QMessageBox::Yes) {
      qApp->restart();
    }
  }

  m_btnApply->setEnabled(false);

  qApp->settings()->setValue(GROUP(GUI), GUI::SettingsWindowInitialSize, size());
}

void FormSettings::cancelSettings() {
  QStringList changed_panels;

  for (SettingsPanel* panel : std::as_const(m_panels)) {
    if (panel->isLoaded() && panel->isDirty()) {
      changed_panels.append(panel->title().toLower());
    }
  }

  if (changed_panels.isEmpty()) {
    reject();
  }
  else {
    const QStringList changed_settings_description =
      changed_panels.replaceInStrings(QRegularExpression(QSL("^")), QString::fromUtf8(QByteArray(" • ")));

    if (MsgBox::show(this,
                     QMessageBox::Icon::Critical,
                     tr("Some settings are changed and will be lost"),
                     tr("Some settings were changed and by cancelling this dialog, you would lose these changes."),
                     tr("Do you really want to close this dialog without saving any settings?"),
                     tr("Changed categories of settings:\n%1.").arg(changed_settings_description.join(QSL(",\n"))),
                     QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                     QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes) {
      reject();
    }
  }
}

void FormSettings::addSettingsPanel(SettingsPanel* panel) {
  QListWidgetItem* itm = new QListWidgetItem(m_ui.m_listSettings);

  itm->setText(panel->title());
  itm->setIcon(panel->icon());

  // m_ui.m_listSettings->addItem(itm);
  m_panels.append(panel);

  QScrollArea* scr = new QScrollArea(m_ui.m_stackedSettings);

  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::Shape::StyledPanel);
  scr->setWidget(panel);

  m_ui.m_stackedSettings->addWidget(scr);

  connect(panel, &SettingsPanel::settingsChanged, this, [this]() {
    m_btnApply->setEnabled(true);
  });
}
