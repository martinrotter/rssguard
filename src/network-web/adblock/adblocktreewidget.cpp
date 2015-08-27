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

#include "network-web/adblock/adblocktreewidget.h"

#include "network-web/adblock/adblocksubscription.h"

#include <QMenu>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>


AdBlockTreeWidget::AdBlockTreeWidget(AdBlockSubscription *subscription, QWidget *parent)
  : QTreeWidget(parent), m_subscription(subscription), m_topItem(NULL),
    m_itemChangingBlock(false), m_refreshAllItemsNeeded(true) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  setHeaderHidden(true);
  setAlternatingRowColors(true);
  setFrameStyle(QFrame::NoFrame);

  connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequested(QPoint)));
  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*)));
  connect(m_subscription, SIGNAL(subscriptionUpdated()), this, SLOT(subscriptionUpdated()));
  connect(m_subscription, SIGNAL(subscriptionError(QString)), this, SLOT(subscriptionError(QString)));
}

AdBlockSubscription *AdBlockTreeWidget::subscription() const {
  return m_subscription;
}

void AdBlockTreeWidget::showRule(const AdBlockRule *rule) {
  if (m_topItem == NULL && rule) {
    // Dialog is not loaded yet. Mark rule for late loading.
    m_ruleToBeSelected = rule->filter();
  }
  else if (!m_ruleToBeSelected.isEmpty()) {
    QList<QTreeWidgetItem*> items = findItems(m_ruleToBeSelected, Qt::MatchRecursive);

    if (!items.isEmpty()) {
      QTreeWidgetItem *item = items.at(0);

      setCurrentItem(item);
      scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }

    m_ruleToBeSelected.clear();
  }
}

void AdBlockTreeWidget::contextMenuRequested(const QPoint &pos) {
  if (!m_subscription->canEditRules()) {
    return;
  }

  QTreeWidgetItem *item = itemAt(pos);

  if (item != NULL) {
    QMenu menu;
    menu.addAction(tr("Add rule"), this, SLOT(addRule()));
    menu.addSeparator();

    QAction *delete_action = menu.addAction(tr("Remove rule"), this, SLOT(removeRule()));

    if (item->parent() == NULL) {
      delete_action->setDisabled(true);
    }

    menu.exec(viewport()->mapToGlobal(pos));
  }
}

void AdBlockTreeWidget::itemChanged(QTreeWidgetItem *item) {
  m_refreshAllItemsNeeded = true;

  if (item == NULL || m_itemChangingBlock) {
    return;
  }

  m_itemChangingBlock = true;

  int offset = item->data(0, Qt::UserRole + 10).toInt();
  const AdBlockRule *old_rle = m_subscription->rule(offset);

  if (item->checkState(0) == Qt::Unchecked && old_rle->isEnabled()) {
    // Disable rule.
    const AdBlockRule *rule = m_subscription->disableRule(offset);

    adjustItemFeatures(item, rule);
  }
  else if (item->checkState(0) == Qt::Checked && !old_rle->isEnabled()) {
    // Enable rule
    const AdBlockRule *rule = m_subscription->enableRule(offset);

    adjustItemFeatures(item, rule);
  }
  else if (m_subscription->canEditRules()) {
    // Custom rule has been changed
    AdBlockRule *new_rule = new AdBlockRule(item->text(0), m_subscription);
    const AdBlockRule *rule = m_subscription->replaceRule(new_rule, offset);

    adjustItemFeatures(item, rule);
  }

  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::copyFilter() {
  QTreeWidgetItem *item = currentItem();

  if (item != NULL) {
    QApplication::clipboard()->setText(item->text(0));
  }
}

void AdBlockTreeWidget::addRule() {
  if (!m_subscription->canEditRules()) {
    return;
  }

  QString new_rule = QInputDialog::getText(this, tr("Add rule"), tr("Please write your rule here"));
  if (new_rule.isEmpty()) {
    return;
  }

  AdBlockRule *rule = new AdBlockRule(new_rule, m_subscription);
  int offset = m_subscription->addRule(rule);

  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, new_rule);
  item->setData(0, Qt::UserRole + 10, offset);
  item->setFlags(item->flags() | Qt::ItemIsEditable);

  m_itemChangingBlock = true;
  m_topItem->addChild(item);
  m_itemChangingBlock = false;

  adjustItemFeatures(item, rule);
}

void AdBlockTreeWidget::removeRule() {
  QTreeWidgetItem *item = currentItem();
  if (item == NULL || !m_subscription->canEditRules() || item == m_topItem) {
    return;
  }

  int offset = item->data(0, Qt::UserRole + 10).toInt();
  m_subscription->removeRule(offset);
  delete item;
}

void AdBlockTreeWidget::subscriptionUpdated() {
  refresh();

  m_itemChangingBlock = true;
  m_topItem->setText(0, tr("%1 (recently updated)").arg(m_subscription->title()));
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::subscriptionError(const QString &message) {
  refresh();

  m_itemChangingBlock = true;
  m_topItem->setText(0, tr("%1 (error: %2)").arg(m_subscription->title(), message));
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::adjustItemFeatures(QTreeWidgetItem *item, const AdBlockRule *rule) {
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

void AdBlockTreeWidget::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) {
    copyFilter();
  }

  if (event->key() == Qt::Key_Delete) {
    removeRule();
  }

  QTreeWidget::keyPressEvent(event);
}

void AdBlockTreeWidget::refresh() {
  // Disable GUI editing for parent.
  emit refreshStatusChanged(true);
  setUpdatesEnabled(false);

  m_itemChangingBlock = true;
  clear();

  QFont bold_font;
  bold_font.setBold(true);

  m_topItem = new QTreeWidgetItem(this);
  m_topItem->setText(0, m_subscription->title());
  m_topItem->setFont(0, bold_font);
  m_topItem->setExpanded(true);
  addTopLevelItem(m_topItem);

  const QVector<AdBlockRule*> &all_rules = m_subscription->allRules();
  int index = 0;

  foreach (const AdBlockRule *rule, all_rules) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_topItem);
    item->setText(0, rule->filter());
    item->setData(0, Qt::UserRole + 10, index);

    if (m_subscription->canEditRules()) {
      item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    adjustItemFeatures(item, rule);
    index++;

    if (index % 100 == 0) {
      qApp->processEvents();
    }
  }

  showRule(0);
  m_itemChangingBlock = false;
  setUpdatesEnabled(true);
  emit refreshStatusChanged(false);
}

void AdBlockTreeWidget::clear() {
  QTreeWidget::clear();
  m_allTreeItems.clear();
}

void AdBlockTreeWidget::addTopLevelItem(QTreeWidgetItem *item) {
  m_allTreeItems.append(item);
  QTreeWidget::addTopLevelItem(item);
}

void AdBlockTreeWidget::iterateAllItems(QTreeWidgetItem *parent)
{
  int count = parent ? parent->childCount() : topLevelItemCount();

  for (int i = 0; i < count; i++) {
    QTreeWidgetItem *item = parent ? parent->child(i) : topLevelItem(i);

    if (item->childCount() == 0) {
      m_allTreeItems.append(item);
    }

    iterateAllItems(item);
  }
}

QList<QTreeWidgetItem*> AdBlockTreeWidget::allItems() {
  if (m_refreshAllItemsNeeded) {
    m_allTreeItems.clear();
    iterateAllItems(0);
    m_refreshAllItemsNeeded = false;
  }

  return m_allTreeItems;
}

void AdBlockTreeWidget::filterString(const QString &string) {
  QList<QTreeWidgetItem*> all_items = allItems();
  QList<QTreeWidgetItem*> parents;
  bool string_empty = string.isEmpty();

  foreach (QTreeWidgetItem *item, all_items) {
    bool contains_string = string_empty || item->text(0).contains(string, Qt::CaseInsensitive);

    if (contains_string) {
      item->setHidden(false);

      if (item->parent() != NULL) {
        if (!parents.contains(item->parent())) {
          parents << item->parent();
        }
      }
    }
    else {
      item->setHidden(true);

      if (item->parent() != NULL) {
        item->parent()->setHidden(true);
      }
    }
  }

  for (int i = 0; i < parents.size(); i++) {
    QTreeWidgetItem *parentItem = parents.at(i);
    parentItem->setHidden(false);
    parentItem->setExpanded(true);

    if (parentItem->parent() != NULL && !parents.contains(parentItem->parent())) {
      parents << parentItem->parent();
    }
  }
}
