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

#include "network-web/adblock/adblocktreewidget.h"

#include "network-web/adblock/adblocksubscription.h"

#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>

AdBlockTreeWidget::AdBlockTreeWidget(AdBlockSubscription* subscription, QWidget* parent)
  : TreeWidget(parent), m_subscription(subscription), m_topItem(nullptr), m_itemChangingBlock(false) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  setDefaultItemShowMode(TreeWidget::ItemShowMode::ItemsExpanded);
  setHeaderHidden(true);
  setAlternatingRowColors(true);
  setLayoutDirection(Qt::LeftToRight);
  setIndentation(5);

  connect(this, &AdBlockTreeWidget::customContextMenuRequested, this, &AdBlockTreeWidget::contextMenuRequested);
  connect(this, &AdBlockTreeWidget::itemChanged, this, &AdBlockTreeWidget::itemChanged);
  connect(m_subscription, &AdBlockSubscription::subscriptionUpdated, this, &AdBlockTreeWidget::subscriptionUpdated);
  connect(m_subscription, &AdBlockSubscription::subscriptionError, this, &AdBlockTreeWidget::subscriptionError);
}

AdBlockSubscription* AdBlockTreeWidget::subscription() const {
  return m_subscription;
}

void AdBlockTreeWidget::showRule(const AdBlockRule* rule) {
  if ((m_topItem == nullptr) && (rule != nullptr)) {
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

void AdBlockTreeWidget::contextMenuRequested(const QPoint& pos) {
  if (!m_subscription->canEditRules()) {
    return;
  }

  QTreeWidgetItem* item = itemAt(pos);

  if (item == nullptr) {
    return;
  }

  QMenu menu;

  menu.addAction(tr("Add rule"), this, &AdBlockTreeWidget::addRule);
  menu.addSeparator();
  QAction* deleteAction = menu.addAction(tr("Remove rule"), this, &AdBlockTreeWidget::removeRule);

  if (item->parent() == nullptr) {
    deleteAction->setDisabled(true);
  }

  menu.exec(viewport()->mapToGlobal(pos));
}

void AdBlockTreeWidget::itemChanged(QTreeWidgetItem* item) {
  if ((item == nullptr) || m_itemChangingBlock) {
    return;
  }

  m_itemChangingBlock = true;
  int offset = item->data(0, Qt::UserRole + 10).toInt();
  const AdBlockRule* oldRule = m_subscription->rule(offset);

  if (item->checkState(0) == Qt::Unchecked && oldRule->isEnabled()) {
    // Disable rule.
    const AdBlockRule* rule = m_subscription->disableRule(offset);

    adjustItemFeatures(item, rule);
  }
  else if (item->checkState(0) == Qt::Checked && !oldRule->isEnabled()) {
    // Enable rule.
    const AdBlockRule* rule = m_subscription->enableRule(offset);

    adjustItemFeatures(item, rule);
  }
  else if (m_subscription->canEditRules()) {
    // Custom rule has been changed.
    AdBlockRule* newRule = new AdBlockRule(item->text(0), m_subscription);
    const AdBlockRule* rule = m_subscription->replaceRule(newRule, offset);

    adjustItemFeatures(item, rule);
  }

  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::copyFilter() {
  QTreeWidgetItem* item = currentItem();

  if (item == nullptr) {
    return;
  }

  QApplication::clipboard()->setText(item->text(0));
}

void AdBlockTreeWidget::addRule() {
  if (!m_subscription->canEditRules()) {
    return;
  }

  QString newRule = QInputDialog::getText(this, tr("Add custom rule"), tr("Please write your rule here:"));

  if (newRule.isEmpty()) {
    return;
  }

  auto* rule = new AdBlockRule(newRule, m_subscription);
  int offset = m_subscription->addRule(rule);
  auto* item = new QTreeWidgetItem();

  item->setText(0, newRule);
  item->setData(0, Qt::UserRole + 10, offset);
  item->setFlags(item->flags() | Qt::ItemIsEditable);
  m_itemChangingBlock = true;
  m_topItem->addChild(item);
  m_itemChangingBlock = false;
  adjustItemFeatures(item, rule);
}

void AdBlockTreeWidget::removeRule() {
  QTreeWidgetItem* item = currentItem();

  if ((item == nullptr) || !m_subscription->canEditRules() || item == m_topItem) {
    return;
  }

  int offset = item->data(0, Qt::UserRole + 10).toInt();

  m_subscription->removeRule(offset);
  deleteItem(item);
}

void AdBlockTreeWidget::subscriptionUpdated() {
  refresh();
  m_itemChangingBlock = true;
  m_topItem->setText(0, tr("%1 (recently updated)").arg(m_subscription->title()));
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::subscriptionError(const QString& message) {
  refresh();
  m_itemChangingBlock = true;
  m_topItem->setText(0, tr("%1 (error: %2)").arg(m_subscription->title(), message));
  m_itemChangingBlock = false;
}

void AdBlockTreeWidget::adjustItemFeatures(QTreeWidgetItem* item, const AdBlockRule* rule) {
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
}

void AdBlockTreeWidget::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier) != 0) {
    copyFilter();
  }

  if (event->key() == Qt::Key_Delete) {
    removeRule();
  }

  TreeWidget::keyPressEvent(event);
}

void AdBlockTreeWidget::refresh() {
  m_itemChangingBlock = true;
  clear();
  QFont boldFont;

  boldFont.setBold(true);
  m_topItem = new QTreeWidgetItem(this);
  m_topItem->setText(0, m_subscription->title());
  m_topItem->setFont(0, boldFont);
  m_topItem->setExpanded(true);
  addTopLevelItem(m_topItem);
  const QVector<AdBlockRule*>& allRules = m_subscription->allRules();
  int index = 0;

  for (const AdBlockRule* rule : allRules) {
    auto* item = new QTreeWidgetItem(m_topItem);

    item->setText(0, rule->filter());
    item->setData(0, Qt::UserRole + 10, index);

    if (m_subscription->canEditRules()) {
      item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    adjustItemFeatures(item, rule);
    ++index;
  }

  showRule(nullptr);
  m_itemChangingBlock = false;
}
