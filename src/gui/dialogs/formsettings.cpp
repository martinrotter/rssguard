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
#include "core/feeddownloader.h"
#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "gui/systemtrayicon.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/feedstoolbar.h"
#include "gui/messagebox.h"
#include "gui/basetoolbar.h"
#include "gui/messagestoolbar.h"
#include "gui/messagesview.h"
#include "gui/statusbar.h"
#include "gui/dialogs/formmain.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"

#include <QProcess>
#include <QNetworkProxy>
#include <QColorDialog>
#include <QFileDialog>
#include <QKeyEvent>
#include <QFontDialog>
#include <QDir>


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings), m_settings(qApp->settings()) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("emblem-system")));

  // Establish needed connections.
  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(saveSettings()));
}

FormSettings::~FormSettings() {
  qDebug("Destroying FormSettings distance.");
}

void FormSettings::promptForRestart() {
  /*if (!m_changedDataTexts.isEmpty()) {
    const QStringList changed_settings_description = m_changedDataTexts.replaceInStrings(QRegExp(QSL("^")), QString::fromUtf8(" • "));
    MessageBox::show(this,
                     QMessageBox::Question,
                     tr("Critical settings were changed"),
                     tr("Some critical settings were changed and will be applied after the application gets restarted. "
                        "\n\nYou have to restart manually."),
                     QString(),
                     tr("List of changes:\n%1.").arg(changed_settings_description .join(QSL(",\n"))),
                     QMessageBox::Ok, QMessageBox::Ok);
  }*/
}

void FormSettings::saveSettings() {
  // Save all settings.
  m_settings->checkSettings();
  promptForRestart();
  accept();
}
