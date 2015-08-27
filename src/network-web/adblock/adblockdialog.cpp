// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/adblock/adblocktreewidget.h"
#include "network-web/adblock/adblockaddsubscriptiondialog.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "gui/tabwidget.h"
#include "gui/dialogs/formmain.h"

#include <QDesktopWidget>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>


AdBlockDialog::AdBlockDialog(QWidget* parent) : QDialog(parent), m_ui(new Ui::AdBlockDialog),
  m_manager(AdBlockManager::instance()), m_currentTreeWidget(NULL),
  m_currentSubscription(NULL), m_loaded(false), m_useLimitedEasyList(false) {
  m_ui->setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->fromTheme("web-adblock"));

  m_ui->m_checkEnable->setChecked(m_manager->isEnabled());
  m_ui->m_checkUseLimitedEasyList->setVisible(false);
  m_ui->m_btnOptions->setIcon(qApp->icons()->fromTheme("web-adblock"));
  m_ui->m_btnOptions->setText(m_ui->m_btnOptions->text() + "   ");

  // Setup the menu.
  setupMenu();

  // Initialize connections.
  createConnections();

  // Load the contents.
  load();
}

AdBlockDialog::~AdBlockDialog() {
  qDebug("Destroying AdBlockDialog instance.");
  delete m_ui;
}

void AdBlockDialog::setupMenu() {
  QMenu *menu = new QMenu(m_ui->m_btnOptions);

  m_actionAddRule = menu->addAction(tr("Add rule"), this, SLOT(addRule()));
  m_actionRemoveRule = menu->addAction(tr("Remove rule"), this, SLOT(removeRule()));
  menu->addSeparator();
  m_actionAddSubscription = menu->addAction(tr("Add subscription"), this, SLOT(addSubscription()));
  m_actionRemoveSubscription = menu->addAction(tr("Remove subscription"), this, SLOT(removeSubscription()));
  menu->addAction(tr("Update subscriptions"), m_manager, SLOT(updateAllSubscriptions()));
  menu->addSeparator();
  menu->addAction(tr("Rules writing guide"), this, SLOT(learnAboutRules()));

  m_ui->m_btnOptions->setMenu(menu);
}

void AdBlockDialog::createConnections() {
  connect(m_ui->m_btnOptions->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowMenu()));
  connect(m_ui->m_checkEnable, SIGNAL(toggled(bool)), this, SLOT(enableAdBlock(bool)));
  connect(m_ui->m_txtFilter, SIGNAL(textChanged(QString)), this, SLOT(filterString(QString)));
  connect(m_ui->m_tabs, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(close()));
}

void AdBlockDialog::showRule(const AdBlockRule *rule) const {
  const AdBlockSubscription *subscription = rule->subscription();

  if (subscription == NULL) {
    return;
  }

  for (int i = 0; i < m_ui->m_tabs->count(); i++) {
    AdBlockTreeWidget *tree_widget = qobject_cast<AdBlockTreeWidget*>(m_ui->m_tabs->widget(i));

    if (subscription == tree_widget->subscription()) {
      tree_widget->showRule(rule);
      m_ui->m_tabs->setCurrentIndex(i);
      tree_widget->setFocus();
      break;
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
  QPointer<AdBlockAddSubscriptionDialog> dialog = new AdBlockAddSubscriptionDialog(this);

  if (dialog.data()->exec() == QDialog::Accepted) {
    QString title = dialog.data()->title();
    QString url = dialog.data()->url();

    if (AdBlockSubscription *subscription = m_manager->addSubscription(title, url)) {
      AdBlockTreeWidget *tree = new AdBlockTreeWidget(subscription, this);

      connect(tree, SIGNAL(refreshStatusChanged(bool)), this, SLOT(setDisabled(bool)));

      int index = m_ui->m_tabs->insertTab(m_ui->m_tabs->count() - 1, tree, subscription->title());

      m_ui->m_tabs->setCurrentIndex(index);
    }
  }

  delete dialog.data();
}

void AdBlockDialog::removeSubscription() {
  if (m_manager->removeSubscription(m_currentSubscription)) {
    delete m_currentTreeWidget;
  }
}

void AdBlockDialog::currentChanged(int index) {
  if (index != -1) {
    m_currentTreeWidget = qobject_cast<AdBlockTreeWidget*>(m_ui->m_tabs->widget(index));
    m_currentSubscription = m_currentTreeWidget->subscription();

    bool is_easylist = m_currentSubscription->url() == QUrl(ADBLOCK_EASYLIST_URL);
    m_ui->m_checkUseLimitedEasyList->setEnabled(is_easylist && m_ui->m_checkEnable->isChecked());
    m_ui->m_checkUseLimitedEasyList->setVisible(is_easylist);

    m_ui->m_txtFilter->blockSignals(true);
    m_ui->m_txtFilter->clear();
    m_ui->m_txtFilter->blockSignals(false);
  }
}

void AdBlockDialog::filterString(const QString &string) {
  if (m_currentTreeWidget && m_ui->m_checkEnable->isChecked()) {
    m_currentTreeWidget->filterString(string);
  }
}

void AdBlockDialog::enableAdBlock(bool state) {
  m_manager->setEnabled(state);

  if (state) {
    load();
  }
}

void AdBlockDialog::aboutToShowMenu() {
  bool subscriptionEditable = m_currentSubscription && m_currentSubscription->canEditRules();
  bool subscriptionRemovable = m_currentSubscription && m_currentSubscription->canBeRemoved();

  m_actionAddRule->setEnabled(subscriptionEditable);
  m_actionRemoveRule->setEnabled(subscriptionEditable);
  m_actionRemoveSubscription->setEnabled(subscriptionRemovable);
}

void AdBlockDialog::learnAboutRules() {
  qApp->mainForm()->tabWidget()->addBrowser(true, true, QUrl(ADBLOCK_FILTERS_HELP));
  QTimer::singleShot(100, this, SLOT(close()));
}

void AdBlockDialog::loadSubscriptions() {
  for (int i = 0; i < m_ui->m_tabs->count(); ++i) {
    qobject_cast<AdBlockTreeWidget*>(m_ui->m_tabs->widget(i))->refresh();
  }
}

void AdBlockDialog::load() {
  if (m_loaded || !m_ui->m_checkEnable->isChecked()) {
    return;
  }

  foreach (AdBlockSubscription *subscription, m_manager->subscriptions()) {
    AdBlockTreeWidget *tree = new AdBlockTreeWidget(subscription, this);

    connect(tree, SIGNAL(refreshStatusChanged(bool)), this, SLOT(setDisabled(bool)));

    m_ui->m_tabs->addTab(tree, subscription->title());
  }

  m_useLimitedEasyList = m_manager->useLimitedEasyList();
  m_ui->m_checkUseLimitedEasyList->setChecked(m_useLimitedEasyList);

  m_loaded = true;

  QTimer::singleShot(100, this, SLOT(loadSubscriptions()));
}

void AdBlockDialog::closeEvent(QCloseEvent *event) {
  if (m_ui->m_checkUseLimitedEasyList->isChecked() != m_useLimitedEasyList) {
    m_manager->setUseLimitedEasyList(m_ui->m_checkUseLimitedEasyList->isChecked());
  }

  QWidget::closeEvent(event);
}
