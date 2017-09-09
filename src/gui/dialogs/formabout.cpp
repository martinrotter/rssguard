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

#include "gui/dialogs/formabout.h"

#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/settingsproperties.h"
#include "gui/guiutilities.h"
#include "exceptions/applicationexception.h"

#include <QFile>
#include <QTextStream>


FormAbout::FormAbout(QWidget* parent) : QDialog(parent) {
	m_ui.setupUi(this);
	m_ui.m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH));
	GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("help-about")), tr("About %1").arg(APP_NAME));
	loadLicenseAndInformation();
	loadSettingsAndPaths();
}

FormAbout::~FormAbout() {
	qDebug("Destroying FormAbout instance.");
}

void FormAbout::loadSettingsAndPaths() {
	if (qApp->settings()->type() == SettingsProperties::Portable) {
		m_ui.m_txtPathsSettingsType->setText(tr("FULLY portable"));
	}
	else {
		m_ui.m_txtPathsSettingsType->setText(tr("NOT portable"));
	}

	m_ui.m_txtPathsDatabaseRoot->setText(QDir::toNativeSeparators(qApp->userDataFolder() +
	                                     QDir::separator() +
	                                     QString(APP_DB_SQLITE_PATH)));
	m_ui.m_txtPathsSettingsFile->setText(QDir::toNativeSeparators(qApp->settings()->fileName()));
	m_ui.m_txtPathsSkinsRoot->setText(QDir::toNativeSeparators(qApp->skins()->customSkinBaseFolder()));
}

void FormAbout::loadLicenseAndInformation() {
	try {
		m_ui.m_txtLicenseGnu->setText(IOFactory::readTextFile(APP_INFO_PATH + QL1S("/COPYING_GNU_GPL_HTML")));
	}
	catch (...) {
		m_ui.m_txtLicenseGnu->setText(tr("License not found."));
	}

	try {
		m_ui.m_txtLicenseGnu->setText(IOFactory::readTextFile(APP_INFO_PATH + QL1S("/COPYING_GNU_GPL_HTML")));
	}
	catch (...) {
		m_ui.m_txtLicenseGnu->setText(tr("License not found."));
	}

	try {
		m_ui.m_txtChangelog->setText(IOFactory::readTextFile(APP_INFO_PATH + QL1S("/CHANGELOG")));
	}
	catch (...) {
		m_ui.m_txtChangelog->setText(tr("Changelog not found."));
	}

	try {
		m_ui.m_txtLicenseBsd->setText(IOFactory::readTextFile(APP_INFO_PATH + QL1S("/COPYING_BSD")));
	}
	catch (...) {
		m_ui.m_txtLicenseBsd->setText(tr("License not found."));
	}

	// Set other informative texts.
	m_ui.m_lblDesc->setText(tr("<b>%8</b><br>"
	                           "<b>Version:</b> %1 (built on %2/%3)<br>"
	                           "<b>Revision:</b> %4<br>"
	                           "<b>Build date:</b> %5<br>"
	                           "<b>Qt:</b> %6 (compiled against %7)<br>").arg(qApp->applicationVersion(),
	                                   APP_SYSTEM_NAME,
	                                   APP_SYSTEM_VERSION,
	                                   APP_REVISION,
	                                   TextFactory::parseDateTime(QString("%1 %2").arg(__DATE__,
	                                           __TIME__)).toString(Qt::DefaultLocaleShortDate),
	                                   qVersion(),
	                                   QT_VERSION_STR,
	                                   APP_NAME));
	m_ui.m_txtInfo->setText(tr("<body>%5 is a (very) tiny feed reader."
	                           "<br><br>This software is distributed under the terms of GNU General Public License, version 3."
	                           "<br><br>Contacts:"
	                           "<ul><li><a href=\"mailto://%1\">%1</a> ~e-mail</li>"
	                           "<li><a href=\"%2\">%2</a> ~website</li></ul>"
	                           "You can obtain source code for %5 from its website."
	                           "<br><br><br>Copyright (C) 2011-%3 %4</body>").arg(APP_EMAIL,
	                                   APP_URL,
	                                   QString::number(QDateTime::currentDateTime().date().year()),
	                                   APP_AUTHOR,
	                                   APP_NAME));
}
