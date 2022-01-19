// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnodejs.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/nodejs.h"

SettingsNodejs::SettingsNodejs(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  m_ui.m_helpInfo->setHelpText(tr("What is Node.js?"),
                               tr("Node.js is asynchronous event-driven JavaScript runtime, designed to build "
                                  "scalable network applications.\n\n"
                                  "%1 integrates Node.js to bring some modern features like Adblock.\n\n"
                                  "Note that usually all required Node.js tools should be available via your \"PATH\" "
                                  "environment variable, so you do not have to specify full paths.").arg(APP_NAME),
                               false);

  m_ui.m_helpPackages->setHelpText(tr("%1 automatically installs some Node.js packages so that you do not have to. %1 does not "
                                      "use global package folder because that requires administrator rights, therefore by default "
                                      "it uses subfolder placed in your \"user data\" folder.").arg(APP_NAME),
                                   false);

  // FOR ME: npm install --prefix "složka"
  // NODE_PATH="složka" node.exe....
}

QString SettingsNodejs::title() const {
  return QSL("Node.js");
}

void SettingsNodejs::loadSettings() {}

void SettingsNodejs::saveSettings() {}
