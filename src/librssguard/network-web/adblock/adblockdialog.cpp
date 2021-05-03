// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "network-web/adblock/adblockdialog.h"

#include "network-web/adblock/adblockaddsubscriptiondialog.h"
#include "network-web/adblock/adblockmanager.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

AdBlockDialog::AdBlockDialog(QWidget* parent)
  : QDialog(parent), m_manager(qApp->web()->adBlock()), m_loaded(false), m_ui(new Ui::AdBlockDialog) {
  m_ui->setupUi(this);
  m_ui->m_cbEnable->setChecked(m_manager->isEnabled());

  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE),
                                      tr("AdBlock configuration"));

  QPushButton* btn_options = m_ui->m_buttonBox->addButton(QDialogButtonBox::FirstButton);

  btn_options->setText(tr("Options"));
  auto* menu = new QMenu(btn_options);

  m_actionAddSubscription = menu->addAction(tr("Add subscription"), this, &AdBlockDialog::addSubscription);
  menu->addSeparator();
  menu->addAction(tr("Learn about writing rules..."), this, &AdBlockDialog::learnAboutRules);

  btn_options->setMenu(menu);

  connect(m_ui->m_cbEnable, &QCheckBox::toggled, this, &AdBlockDialog::enableAdBlock);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &AdBlockDialog::close);

  load();
  m_ui->m_buttonBox->setFocus();
}

void AdBlockDialog::addSubscription() {
  AdBlockAddSubscriptionDialog dialog(this);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  QString url = dialog.url();

  // TODO: add filter list.
}

void AdBlockDialog::enableAdBlock(bool state) {
  m_manager->load(false);

  if (state) {
    load();
  }
}

void AdBlockDialog::learnAboutRules() {
  qApp->web()->openUrlInExternalBrowser(QSL(ADBLOCK_HOWTO));
}

void AdBlockDialog::load() {
  if (m_loaded || !m_ui->m_cbEnable->isChecked()) {
    return;
  }

  // TODO: load
}
