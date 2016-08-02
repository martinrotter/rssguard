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

#include "gui/settings/settingsfeedsmessages.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagesview.h"

#include <QFontDialog>


SettingsFeedsMessages::SettingsFeedsMessages(Settings *settings, QWidget *parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsFeedsMessages){
  m_ui->setupUi(this);
  initializeMessageDateFormats();
  connect(m_ui->m_btnChangeMessagesFont, &QPushButton::clicked, this, &SettingsFeedsMessages::changeMessagesFont);
}

SettingsFeedsMessages::~SettingsFeedsMessages() {
  delete m_ui;
}


void SettingsFeedsMessages::initializeMessageDateFormats() {
  QStringList best_formats; best_formats << QSL("d/M/yyyy hh:mm:ss") << QSL("ddd, d. M. yy hh:mm:ss") <<
                                            QSL("yyyy-MM-dd HH:mm:ss.z") << QSL("yyyy-MM-ddThh:mm:ss") <<
                                            QSL("MMM d yyyy hh:mm:ss");;
  const QLocale current_locale = qApp->localization()->loadedLocale();
  const QDateTime current_dt = QDateTime::currentDateTime();

  foreach (const QString &format, best_formats) {
    m_ui->m_cmbMessagesDateTimeFormat->addItem(current_locale.toString(current_dt, format), format);
  }
}

void SettingsFeedsMessages::changeMessagesFont() {
  bool ok;
  QFont new_font = QFontDialog::getFont(&ok, m_ui->m_lblMessagesFont->font(),
                                        this, tr("Select new font for message viewer"),
                                        QFontDialog::DontUseNativeDialog);

  if (ok) {
    m_ui->m_lblMessagesFont->setFont(new_font);
  }
}

void SettingsFeedsMessages::loadSettings() {
  m_ui->m_checkKeppMessagesInTheMiddle->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool());
  m_ui->m_checkRemoveReadMessagesOnExit->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool());
  m_ui->m_checkAutoUpdate->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool());
  m_ui->m_spinAutoUpdateInterval->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt());
  m_ui->m_spinFeedUpdateTimeout->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt());
  m_ui->m_checkUpdateAllFeedsOnStartup->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool());
  m_ui->m_cmbCountsFeedList->addItems(QStringList() << "(%unread)" << "[%unread]" << "%unread/%all" << "%unread-%all" << "[%unread|%all]");
  m_ui->m_cmbCountsFeedList->setEditText(settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString());

  initializeMessageDateFormats();

  m_ui->m_checkMessagesDateTimeFormat->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool());
  const int index_format = m_ui->m_cmbMessagesDateTimeFormat->findData(settings()->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString());

  if (index_format >= 0) {
    m_ui->m_cmbMessagesDateTimeFormat->setCurrentIndex(index_format);
  }

  m_ui->m_lblMessagesFont->setText(tr("Font preview"));
  QFont fon;
  fon.fromString(settings()->value(GROUP(Messages),
                                   SETTING(Messages::PreviewerFontStandard)).toString());
  m_ui->m_lblMessagesFont->setFont(fon);
}

void SettingsFeedsMessages::saveSettings() {
  settings()->setValue(GROUP(Messages), Messages::KeepCursorInCenter, m_ui->m_checkKeppMessagesInTheMiddle->isChecked());
  settings()->setValue(GROUP(Messages), Messages::ClearReadOnExit, m_ui->m_checkRemoveReadMessagesOnExit->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateEnabled, m_ui->m_checkAutoUpdate->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateInterval, m_ui->m_spinAutoUpdateInterval->value());
  settings()->setValue(GROUP(Feeds), Feeds::UpdateTimeout, m_ui->m_spinFeedUpdateTimeout->value());
  settings()->setValue(GROUP(Feeds), Feeds::FeedsUpdateOnStartup, m_ui->m_checkUpdateAllFeedsOnStartup->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::CountFormat, m_ui->m_cmbCountsFeedList->currentText());
  settings()->setValue(GROUP(Messages), Messages::UseCustomDate, m_ui->m_checkMessagesDateTimeFormat->isChecked());
  settings()->setValue(GROUP(Messages), Messages::CustomDateFormat,
                       m_ui->m_cmbMessagesDateTimeFormat->itemData(m_ui->m_cmbMessagesDateTimeFormat->currentIndex()).toString());

  // Save fonts.
  settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, m_ui->m_lblMessagesFont->font().toString());

  qApp->mainForm()->tabWidget()->feedMessageViewer()->loadMessageViewerFonts();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->updateAutoUpdateStatus();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->reloadWholeLayout();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesView()->sourceModel()->updateDateFormat();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesView()->sourceModel()->reloadWholeLayout();
}
