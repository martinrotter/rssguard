// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnodejs.h"

#include "definitions/definitions.h"

SettingsNodejs::SettingsNodejs(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  m_ui.m_helpInfo->setHelpText(tr("What is Node.js?"),
                               tr("Node.js is asynchronous event-driven JavaScript runtime, designed to build "
                                  "scalable network applications.\n\n"
                                  "%1 integrates Node.js to bring some modern features like Adblock.").arg(APP_NAME),
                               false);
}

QString SettingsNodejs::title() const {
  return QSL("Node.js");
}

void SettingsNodejs::loadSettings()
{}

void SettingsNodejs::saveSettings()
{}
