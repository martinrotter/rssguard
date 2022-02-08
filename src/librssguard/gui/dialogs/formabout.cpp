// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formabout.h"

#include "database/databasedriver.h"
#include "database/databasefactory.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settingsproperties.h"
#include "miscellaneous/textfactory.h"

#include <QFile>
#include <QTextStream>

FormAbout::FormAbout(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);
  m_ui.m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH));
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("help-about")), tr("About %1").arg(QSL(APP_NAME)));
  loadLicenseAndInformation();
  loadSettingsAndPaths();
}

FormAbout::~FormAbout() {}

void FormAbout::loadSettingsAndPaths() {
  if (qApp->settings()->type() == SettingsProperties::SettingsType::Portable) {
    m_ui.m_txtPathsSettingsType->setText(tr("FULLY portable"));
  }
  else if (qApp->settings()->type() == SettingsProperties::SettingsType::Custom) {
    m_ui.m_txtPathsSettingsType->setText(tr("CUSTOM"));
  }
  else {
    m_ui.m_txtPathsSettingsType->setText(tr("NOT portable"));
  }

  m_ui.m_txtPathsDatabaseRoot->setText(qApp->database()->driver()->location());
  m_ui.m_txtPathsSettingsFile->setText(QDir::toNativeSeparators(qApp->settings()->fileName()));
  m_ui.m_txtPathsSkinsRoot->setText(QDir::toNativeSeparators(qApp->skins()->customSkinBaseFolder()));
}

void FormAbout::loadLicenseAndInformation() {
  try {
    m_ui.m_txtLicenseGnuGpl->setText(IOFactory::readFile(APP_INFO_PATH + QL1S("/COPYING_GNU_GPL_HTML")));
  }
  catch (...) {
    m_ui.m_txtLicenseGnuGpl->setText(tr("License not found."));
  }

  try {
    m_ui.m_txtLicenseGnuLgpl->setText(IOFactory::readFile(APP_INFO_PATH + QL1S("/COPYING_GNU_LGPL")));
  }
  catch (...) {
    m_ui.m_txtLicenseGnuLgpl->setText(tr("License not found."));
  }

  try {
    m_ui.m_txtChangelog->setText(IOFactory::readFile(APP_INFO_PATH + QL1S("/CHANGELOG")));
  }
  catch (...) {
    m_ui.m_txtChangelog->setText(tr("Changelog not found."));
  }

  try {
    m_ui.m_txtLicenseMit->setText(IOFactory::readFile(APP_INFO_PATH + QL1S("/COPYING_MIT")));
  }
  catch (...) {
    m_ui.m_txtLicenseMit->setText(tr("License not found."));
  }

  // Set other informative texts.
  m_ui.m_lblDesc->setText(tr("<b>%8</b><br>"
                             "<b>Version:</b> %1 (built on %2/%3)<br>"
                             "<b>Revision:</b> %4<br>"
                             "<b>Build date:</b> %5<br>"
                             "<b>Qt:</b> %6 (compiled against %7)<br>").arg(
                            qApp->applicationVersion(), QSL(APP_SYSTEM_NAME),
                            QSL(APP_SYSTEM_VERSION), QSL(APP_REVISION),
                            qApp->localization()->loadedLocale().toString(TextFactory::parseDateTime(QSL("%1 %2").arg(__DATE__,
                                                                                                                      __TIME__)),
                                                                          QLocale::FormatType::ShortFormat),
                            qVersion(), QSL(QT_VERSION_STR),
                            QSL(APP_NAME)));
  m_ui.m_txtInfo->setText(tr("<body>%5 is a (very) tiny feed reader."
                             "<br><br>This software is distributed under the terms of GNU General Public License, version 3."
                             "<br><br>Contacts:"
                             "<ul><li><a href=\"mailto://%1\">%1</a> ~e-mail</li>"
                             "<li><a href=\"%2\">%2</a> ~website</li></ul>"
                             "You can obtain source code for %5 from its website."
                             "<br><br><br>Copyright (C) 2011-%3 %4</body>").arg(QSL(APP_EMAIL), QSL(APP_URL),
                                                                                QString::number(QDateTime::currentDateTime()
                                                                                                .date()
                                                                                                .year()),
                                                                                QSL(APP_AUTHOR), QSL(APP_NAME)));
}
