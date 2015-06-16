/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2015 QuiteRSS Team <quiterssteam@gmail.com>
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
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
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
#include "adblocktreewidget.h"
#include "adblocksubscription.h"

#include <QMenu>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>

AdBlockTreeWidget::AdBlockTreeWidget(AdBlockSubscription* subscription, QWidget* parent)
  : QTreeWidget(parent)
  , m_subscription(subscription)
  , m_topItem(0)
  , m_itemChangingBlock(false)
  , m_refreshAllItemsNeeded(true)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setHeaderHidden(true);
  setAlternatingRowColors(true);
  setFrameStyle(QFrame::NoFrame);

  connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequested(QPoint)));
  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*)));
  connect(m_subscription, SIGNAL(subscriptionUpdated()), this, SLOT(subscriptionUpdated()));
  connect(m_subscription, SIGNAL(subscriptionError(QString)), this, SLOT(subscriptionError(QString)));
}

AdBlockSubscription* AdBlockTreeWidget::subscription() const
{
  return m_subscription;
}

void AdBlockTreeWidget::showRule(const AdBlockRule* rule)
{
  if (!m_topItem && rule) {
    m_ruleToBeSelected = rule->filter();
  }
  else if (!m_ruleToBeSelected.isEmpty()) {
    QList<QTreeWidgetItem*> items = findItems(m_ruleToBeSelected, Qt::MatchRecursive);
    if (!items.isEmpty()) {
      QTreeWidgetItem* item = items.at(0);

      setCurrentItem(item);
      scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }

    m_ruleToBeSelected.clear();
  }
}

void AdBlockTreeWidget::contextMenuRequested(const QPoint &pos)
{
  if (!m_subscription->canEditRules()) {
    return;
  }

  QTreeWidgetItem* item = itemAt(pos);
  if (!item) {
    return;
  }

  QMenu menu;
  menu.addAction(tr("Add Rule"), this, SLOT(addRule()));
  menu.addSeparator();
  QAction* deleteAction = menu.addAction(tr("Remove Rule"), this, SLOT(removeRule()));

  if (!item->parent()) {
    deleteAction->setDisabled(true);
  }

  menu.exec(viewport()->mapToGlobal(pos));
}

void AdBlockTreeWidget::itemChanged(QTreeWidgetItem* item)
{
  m_refreshAllItemsNeeded = true;

  if (!item || m_itemChangingBlock) {
    return;
  }

  m_itemChangingBlock = true;

  int offset = item->data(0, Qt::UserRole + 10).toInt();
  const AdBlockRule* oldRule = m_subscription->rule(offset);

  if (item->checkState(0) == Qt::Unchecked && oldRule->isEnabled()) {
    // Disable rule
    const AdBlockRule* rule = m_subscription->disableRule(offset);

    adjustItemFeatures(item, rule);
  }
  else if (item->checkState(0) == Qt::Checked && !oldRule->isEnabled()) {
    // Enable rule
    const AdBlockRule* rule = m_subscription->enableRule(offset);

    adjustItemFeatures(item, rule);
  }
  else if (m_subscription->canEditRules()) {
    // Custom rule has been changed
    AdBlockRule* newRule = new AdBlockRule(item->text(0), m_subscription);
    const AdBlockRule* rule = m_subscription->replaceRule(newRule, offset);

    adjustItemFeatures(item, rule);
  }

  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::copyFilter()
{
  QTreeWidgetItem* item = currentItem();
  if (!item) {
    return;
  }

  QApplication::clipboard()->setText(item->text(0));
}

void AdBlockTreeWidget::addRule()
{
  if (!m_subscription->canEditRules()) {
    return;
  }

  QString newRule = QInputDialog::getText(this, tr("Add Custom Rule"), tr("Please write your rule here:"));
  if (newRule.isEmpty()) {
    return;
  }

  AdBlockRule* rule = new AdBlockRule(newRule, m_subscription);
  int offset = m_subscription->addRule(rule);

  QTreeWidgetItem* item = new QTreeWidgetItem();
  item->setText(0, newRule);
  item->setData(0, Qt::UserRole + 10, offset);
  item->setFlags(item->flags() | Qt::ItemIsEditable);

  m_itemChangingBlock = true;
  m_topItem->addChild(item);
  m_itemChangingBlock = false;

  adjustItemFeatures(item, rule);
}

void AdBlockTreeWidget::removeRule()
{
  QTreeWidgetItem* item = currentItem();
  if (!item || !m_subscription->canEditRules() || item == m_topItem) {
    return;
  }

  int offset = item->data(0, Qt::UserRole + 10).toInt();

  m_subscription->removeRule(offset);
  delete item;
}

void AdBlockTreeWidget::subscriptionUpdated()
{
  refresh();

  m_itemChangingBlock = true;
  m_topItem->setText(0, tr("%1 (recently updated)").arg(m_subscription->title()));
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::subscriptionError(const QString &message)
{
  refresh();

  m_itemChangingBlock = true;
  m_topItem->setText(0, tr("%1 (Error: %2)").arg(m_subscription->title(), message));
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::adjustItemFeatures(QTreeWidgetItem* item, const AdBlockRule* rule)
{
  if (!rule->isEnabled()) {
    QFont font;
    font.setItalic(true);
    item->setForeground(0, QColor(Qt::gray));

    if (!rule->isComment()) {
      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      item->setCheckState(0, Qt::Unchecked);
      item->setFont(0, font);
    }

    return;
  }

  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(0, Qt::Checked);

  if (rule->isException()) {
    item->setForeground(0, QColor(Qt::darkGreen));
    item->setFont(0, QFont());
  }
  else if (rule->isCssRule()) {
    item->setForeground(0, QColor(Qt::darkBlue));
    item->setFont(0, QFont());
  }
  else {
    item->setForeground(0, QColor());
    item->setFont(0, QFont());
  }
}

void AdBlockTreeWidget::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) {
    copyFilter();
  }

  if (event->key() == Qt::Key_Delete) {
    removeRule();
  }

  QTreeWidget::keyPressEvent(event);
}

void AdBlockTreeWidget::refresh()
{
  m_itemChangingBlock = true;
  clear();

  QFont boldFont;
  boldFont.setBold(true);

  m_topItem = new QTreeWidgetItem(this);
  m_topItem->setText(0, m_subscription->title());
  m_topItem->setFont(0, boldFont);
  m_topItem->setExpanded(true);
  addTopLevelItem(m_topItem);

  const QVector<AdBlockRule*> &allRules = m_subscription->allRules();

  int index = 0;
  foreach (const AdBlockRule* rule, allRules) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_topItem);
    item->setText(0, rule->filter());
    item->setData(0, Qt::UserRole + 10, index);

    if (m_subscription->canEditRules()) {
      item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    adjustItemFeatures(item, rule);
    ++index;
  }

  showRule(0);
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::clear()
{
  QTreeWidget::clear();
  m_allTreeItems.clear();
}

void AdBlockTreeWidget::addTopLevelItem(QTreeWidgetItem* item)
{
  m_allTreeItems.append(item);
  QTreeWidget::addTopLevelItem(item);
}

void AdBlockTreeWidget::iterateAllItems(QTreeWidgetItem* parent)
{
  int count = parent ? parent->childCount() : topLevelItemCount();

  for (int i = 0; i < count; i++) {
    QTreeWidgetItem* item = parent ? parent->child(i) : topLevelItem(i);

    if (item->childCount() == 0) {
      m_allTreeItems.append(item);
    }

    iterateAllItems(item);
  }
}

QList<QTreeWidgetItem*> AdBlockTreeWidget::allItems()
{
  if (m_refreshAllItemsNeeded) {
    m_allTreeItems.clear();
    iterateAllItems(0);
    m_refreshAllItemsNeeded = false;
  }

  return m_allTreeItems;
}

void AdBlockTreeWidget::filterString(const QString &string)
{
  QList<QTreeWidgetItem*> _allItems = allItems();
  QList<QTreeWidgetItem*> parents;
  bool stringIsEmpty = string.isEmpty();
  foreach (QTreeWidgetItem* item, _allItems) {
    bool containsString = stringIsEmpty || item->text(0).contains(string, Qt::CaseInsensitive);
    if (containsString) {
      item->setHidden(false);
      if (item->parent()) {
        if (!parents.contains(item->parent())) {
          parents << item->parent();
        }
      }
    }
    else {
      item->setHidden(true);
      if (item->parent()) {
        item->parent()->setHidden(true);
      }
    }
  }

  for (int i = 0; i < parents.size(); ++i) {
    QTreeWidgetItem* parentItem = parents.at(i);
    parentItem->setHidden(false);
    parentItem->setExpanded(true);

    if (parentItem->parent() && !parents.contains(parentItem->parent())) {
      parents << parentItem->parent();
    }
  }
}
