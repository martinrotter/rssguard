/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2010-2017 David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "adblockdialog.h"
#include "adblockmanager.h"
#include "adblocksubscription.h"
#include "adblocktreewidget.h"
#include "adblockaddsubscriptiondialog.h"
#include "mainapplication.h"
#include "qztools.h"

#include <QMenu>
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>

AdBlockDialog::AdBlockDialog(QWidget* parent)
    : QWidget(parent)
    , m_manager(AdBlockManager::instance())
    , m_currentTreeWidget(0)
    , m_currentSubscription(0)
    , m_loaded(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setupUi(this);

    QzTools::centerWidgetOnScreen(this);

#ifdef Q_OS_MACOS
    tabWidget->setDocumentMode(false);
#endif
    adblockCheckBox->setChecked(m_manager->isEnabled());

    QMenu* menu = new QMenu(buttonOptions);
    m_actionAddRule = menu->addAction(tr("Add Rule"), this, SLOT(addRule()));
    m_actionRemoveRule = menu->addAction(tr("Remove Rule"), this, SLOT(removeRule()));
    menu->addSeparator();
    m_actionAddSubscription = menu->addAction(tr("Add Subscription"), this, SLOT(addSubscription()));
    m_actionRemoveSubscription = menu->addAction(tr("Remove Subscription"), this, SLOT(removeSubscription()));
    menu->addAction(tr("Update Subscriptions"), m_manager, SLOT(updateAllSubscriptions()));
    menu->addSeparator();
    menu->addAction(tr("Learn about writing rules..."), this, SLOT(learnAboutRules()));

    buttonOptions->setMenu(menu);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowMenu()));

    connect(adblockCheckBox, SIGNAL(toggled(bool)), this, SLOT(enableAdBlock(bool)));
    connect(search, SIGNAL(textChanged(QString)), this, SLOT(filterString(QString)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));

    load();

    buttonBox->setFocus();
}

void AdBlockDialog::showRule(const AdBlockRule* rule) const
{
    AdBlockSubscription* subscription = rule->subscription();
    if (!subscription) {
        return;
    }

    for (int i = 0; i < tabWidget->count(); ++i) {
        AdBlockTreeWidget* treeWidget = qobject_cast<AdBlockTreeWidget*>(tabWidget->widget(i));

        if (subscription == treeWidget->subscription()) {
            treeWidget->showRule(rule);
            tabWidget->setCurrentIndex(i);
            break;
        }
    }
}

void AdBlockDialog::addRule()
{
    m_currentTreeWidget->addRule();
}

void AdBlockDialog::removeRule()
{
    m_currentTreeWidget->removeRule();
}

void AdBlockDialog::addSubscription()
{
    AdBlockAddSubscriptionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString title = dialog.title();
    QString url = dialog.url();

    if (AdBlockSubscription* subscription = m_manager->addSubscription(title, url)) {
        AdBlockTreeWidget* tree = new AdBlockTreeWidget(subscription, tabWidget);
        int index = tabWidget->insertTab(tabWidget->count() - 1, tree, subscription->title());

        tabWidget->setCurrentIndex(index);
    }
}

void AdBlockDialog::removeSubscription()
{
    if (m_manager->removeSubscription(m_currentSubscription)) {
        delete m_currentTreeWidget;
    }
}

void AdBlockDialog::currentChanged(int index)
{
    if (index != -1) {
        m_currentTreeWidget = qobject_cast<AdBlockTreeWidget*>(tabWidget->widget(index));
        m_currentSubscription = m_currentTreeWidget->subscription();
    }
}

void AdBlockDialog::filterString(const QString &string)
{
    if (m_currentTreeWidget && adblockCheckBox->isChecked()) {
        m_currentTreeWidget->filterString(string);
    }
}

void AdBlockDialog::enableAdBlock(bool state)
{
    m_manager->setEnabled(state);

    if (state) {
        load();
    }
}

void AdBlockDialog::aboutToShowMenu()
{
    bool subscriptionEditable = m_currentSubscription && m_currentSubscription->canEditRules();
    bool subscriptionRemovable = m_currentSubscription && m_currentSubscription->canBeRemoved();

    m_actionAddRule->setEnabled(subscriptionEditable);
    m_actionRemoveRule->setEnabled(subscriptionEditable);
    m_actionRemoveSubscription->setEnabled(subscriptionRemovable);
}

void AdBlockDialog::learnAboutRules()
{
    mApp->addNewTab(QUrl("http://adblockplus.org/en/filters"));
}

void AdBlockDialog::loadSubscriptions()
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        AdBlockTreeWidget* treeWidget = qobject_cast<AdBlockTreeWidget*>(tabWidget->widget(i));
        treeWidget->refresh();
    }
}

void AdBlockDialog::load()
{
    if (m_loaded || !adblockCheckBox->isChecked()) {
        return;
    }

    foreach (AdBlockSubscription* subscription, m_manager->subscriptions()) {
        AdBlockTreeWidget* tree = new AdBlockTreeWidget(subscription, tabWidget);
        tabWidget->addTab(tree, subscription->title());
    }

    m_loaded = true;

    QTimer::singleShot(50, this, SLOT(loadSubscriptions()));
}
