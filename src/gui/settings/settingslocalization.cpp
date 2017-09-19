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

#include "gui/settings/settingslocalization.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/settings.h"

SettingsLocalization::SettingsLocalization(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsLocalization) {
  m_ui->setupUi(this);
  m_ui->m_treeLanguages->setColumnCount(3);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << /*: Language column of language list. */ tr("Language")
                                         << /*: Lang. code column of language list. */ tr("Code")
                                         << tr("Author"));

  // Setup languages.
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  connect(m_ui->m_treeLanguages, &QTreeWidget::currentItemChanged, this, &SettingsLocalization::requireRestart);
  connect(m_ui->m_treeLanguages, &QTreeWidget::currentItemChanged, this, &SettingsLocalization::dirtifySettings);
}

SettingsLocalization::~SettingsLocalization() {
  delete m_ui;
}

void SettingsLocalization::loadSettings() {
  onBeginLoadSettings();

  foreach (const Language& language, qApp->localization()->installedLanguages()) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->m_treeLanguages);

    item->setText(0, language.m_name);
    item->setText(1, language.m_code);
    item->setText(2, language.m_author);
    item->setIcon(0, qApp->icons()->miscIcon(QString(FLAG_ICON_SUBFOLDER) + QDir::separator() + language.m_code));
  }

  m_ui->m_treeLanguages->sortByColumn(0, Qt::AscendingOrder);
  QList<QTreeWidgetItem*> matching_items = m_ui->m_treeLanguages->findItems(qApp->localization()->loadedLanguage(), Qt::MatchContains, 1);

  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }

  onEndLoadSettings();
}

void SettingsLocalization::saveSettings() {
  onBeginSaveSettings();

  if (m_ui->m_treeLanguages->currentItem() == nullptr) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  const QString actual_lang = qApp->localization()->loadedLanguage();
  const QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  // Save prompt for restart if language has changed.
  if (new_lang != actual_lang) {
    requireRestart();
    settings()->setValue(GROUP(General), General::Language, new_lang);
  }

  onEndSaveSettings();
}
