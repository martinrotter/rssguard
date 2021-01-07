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
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/adblock/adblocktreewidget.h"

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
  : QDialog(parent), m_manager(qApp->web()->adBlock()), m_currentTreeWidget(nullptr), m_currentSubscription(nullptr),
  m_loaded(false), m_ui(new Ui::AdBlockDialog) {
  m_ui->setupUi(this);
  m_ui->m_cbEnable->setChecked(m_manager->isEnabled());

  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE),
                                      tr("AdBlock configuration"));

  QPushButton* btn_options = m_ui->m_buttonBox->addButton(QDialogButtonBox::FirstButton);

  btn_options->setText(tr("Options"));
  auto* menu = new QMenu(btn_options);

  m_actionAddRule = menu->addAction(tr("Add rule"), this, &AdBlockDialog::addRule);
  m_actionRemoveRule = menu->addAction(tr("Remove rule"), this, &AdBlockDialog::removeRule);
  menu->addSeparator();
  m_actionAddSubscription = menu->addAction(tr("Add subscription"), this, &AdBlockDialog::addSubscription);
  m_actionRemoveSubscription = menu->addAction(tr("Remove subscription"), this, &AdBlockDialog::removeSubscription);
  menu->addAction(tr("Update subscriptions"), m_manager, &AdBlockManager::updateAllSubscriptions);
  menu->addSeparator();
  menu->addAction(tr("Learn about writing rules..."), this, &AdBlockDialog::learnAboutRules);

  btn_options->setMenu(menu);

  connect(menu, &QMenu::aboutToShow, this, &AdBlockDialog::aboutToShowMenu);
  connect(m_ui->m_cbEnable, &QCheckBox::toggled, this, &AdBlockDialog::enableAdBlock);
  connect(m_ui->m_tabSubscriptions, &QTabWidget::currentChanged, this, &AdBlockDialog::currentChanged);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &AdBlockDialog::close);

  load();
  m_ui->m_buttonBox->setFocus();
}

void AdBlockDialog::showRule(const AdBlockRule* rule) const {
  AdBlockSubscription* subscription = rule->subscription();

  if (subscription != nullptr) {
    for (int i = 0; i < m_ui->m_tabSubscriptions->count(); ++i) {
      auto* tree_widget = qobject_cast<AdBlockTreeWidget*>(m_ui->m_tabSubscriptions->widget(i));

      if (subscription == tree_widget->subscription()) {
        tree_widget->showRule(rule);
        m_ui->m_tabSubscriptions->setCurrentIndex(i);
        break;
      }
    }
  }
}

void AdBlockDialog::addRule() {
  m_currentTreeWidget->addRule();
}

void AdBlockDialog::removeRule() {
  m_currentTreeWidget->removeRule();
}

void AdBlockDialog::addSubscription() {
  AdBlockAddSubscriptionDialog dialog(this);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  QString title = dialog.title();
  QString url = dialog.url();

  if (AdBlockSubscription* subscription = m_manager->addSubscription(title, url)) {
    auto* tree = new AdBlockTreeWidget(subscription, m_ui->m_tabSubscriptions);
    int index = m_ui->m_tabSubscriptions->insertTab(m_ui->m_tabSubscriptions->count() - 1, tree, subscription->title());

    m_ui->m_tabSubscriptions->setCurrentIndex(index);
  }
}

void AdBlockDialog::removeSubscription() {
  if (m_manager->removeSubscription(m_currentSubscription)) {
    delete m_currentTreeWidget;
  }
}

void AdBlockDialog::currentChanged(int index) {
  if (index != -1) {
    m_currentTreeWidget = qobject_cast<AdBlockTreeWidget*>(m_ui->m_tabSubscriptions->widget(index));
    m_currentSubscription = m_currentTreeWidget->subscription();
  }
}

void AdBlockDialog::enableAdBlock(bool state) {
  m_manager->load(false);

  if (state) {
    load();
  }
}

void AdBlockDialog::aboutToShowMenu() {
  bool subscriptionEditable = (m_currentSubscription != nullptr) && m_currentSubscription->canEditRules();
  bool subscriptionRemovable = (m_currentSubscription != nullptr) && m_currentSubscription->canBeRemoved();

  m_actionAddRule->setEnabled(subscriptionEditable);
  m_actionRemoveRule->setEnabled(subscriptionEditable);
  m_actionRemoveSubscription->setEnabled(subscriptionRemovable);
}

void AdBlockDialog::learnAboutRules() {
  qApp->web()->openUrlInExternalBrowser(QSL(ADBLOCK_HOWTO_FILTERS));
}

void AdBlockDialog::loadSubscriptions() {
  for (int i = 0; i < m_ui->m_tabSubscriptions->count(); ++i) {
    auto* tree_widget = qobject_cast<AdBlockTreeWidget*>(m_ui->m_tabSubscriptions->widget(i));

    tree_widget->refresh();
  }
}

void AdBlockDialog::load() {
  if (m_loaded || !m_ui->m_cbEnable->isChecked()) {
    return;
  }

  for (AdBlockSubscription* subscription : m_manager->subscriptions()) {
    auto* tree = new AdBlockTreeWidget(subscription, m_ui->m_tabSubscriptions);

    m_ui->m_tabSubscriptions->addTab(tree, subscription->title());
  }

  m_loaded = true;
  QTimer::singleShot(50, this, &AdBlockDialog::loadSubscriptions);
}
