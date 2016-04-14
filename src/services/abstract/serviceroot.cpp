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

#include "services/abstract/serviceroot.h"

#include "core/feedsmodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/databasequeries.h"
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/recyclebin.h"

#include <QSqlTableModel>


ServiceRoot::ServiceRoot(RootItem *parent) : RootItem(parent), m_accountId(NO_PARENT_CATEGORY) {
  setKind(RootItemKind::ServiceRoot);
  setCreationDate(QDateTime::currentDateTime());
}

ServiceRoot::~ServiceRoot() {
}

bool ServiceRoot::deleteViaGui() {
  QSqlDatabase database= qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::deleteAccount(database, accountId())) {
    requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

bool ServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::markAccountReadUnread(database, accountId(), status)) {
    updateCounts(false);
    itemChanged(getSubTree());
    requestReloadMessageList(status == RootItem::Read);
    return true;
  }
  else {
    return false;
  }
}

QList<QAction*> ServiceRoot::addItemMenu() {
  return QList<QAction*>();
}

QList<QAction*> ServiceRoot::contextMenu() {
  return serviceMenu();
}

QList<QAction*> ServiceRoot::serviceMenu() {
  return QList<QAction*>();
}

void ServiceRoot::updateCounts(bool including_total_count) {
  QList<Feed*> feeds;

  foreach (RootItem *child, getSubTree()) {
    if (child->kind() == RootItemKind::Feed) {
      feeds.append(child->toFeed());
    }
    else if (child->kind() != RootItemKind::Category && child->kind() != RootItemKind::ServiceRoot) {
      child->updateCounts(including_total_count);
    }
  }

  if (feeds.isEmpty()) {
    return;
  }

  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  bool ok;
  QMap<int,QPair<int,int> > counts = DatabaseQueries::getMessageCountsForAccount(database, accountId(), including_total_count, &ok);

  foreach (Feed *feed, feeds) {
    feed->setCountOfUnreadMessages(counts.value(feed->customId()).first);

    if (including_total_count) {
      feed->setCountOfAllMessages(counts.value(feed->customId()).second);
    }
  }
}

void ServiceRoot::completelyRemoveAllData() {
  // Purge old data from SQL and clean all model items.
  removeOldFeedTree(true);
  cleanAllItems();
  updateCounts(true);
  itemChanged(QList<RootItem*>() << this);
  requestReloadMessageList(true);
}

void ServiceRoot::removeOldFeedTree(bool including_messages) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  DatabaseQueries::deleteAccountData(database, accountId(), including_messages);
}

void ServiceRoot::cleanAllItems() {
  foreach (RootItem *top_level_item, childItems()) {
    if (top_level_item->kind() != RootItemKind::Bin) {
      requestItemRemoval(top_level_item);
    }
  }
}

bool ServiceRoot::cleanFeeds(QList<Feed*> items, bool clean_read_only) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::cleanFeeds(database, textualFeedIds(items), clean_read_only, accountId())) {
    // Messages are cleared, now inform model about need to reload data.
    QList<RootItem*> itemss;

    foreach (Feed *feed, items) {
      feed->updateCounts(true);
      itemss.append(feed);
    }

    RecycleBin *bin = recycleBin();

    if (bin != NULL) {
      bin->updateCounts(true);
      itemss.append(bin);
    }

    itemChanged(itemss);
    requestReloadMessageList(true);
    return true;
  }
  else {
    return false;
  }
}

void ServiceRoot::storeNewFeedTree(RootItem *root) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::storeAccountTree(database, root, accountId())) {
    RecycleBin *bin = recycleBin();

    if (bin != NULL && !childItems().contains(bin)) {
      // As the last item, add recycle bin, which is needed.
      appendChild(bin);
      bin->updateCounts(true);
    }
  }
}

void ServiceRoot::removeLeftOverMessages() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  DatabaseQueries::deleteLeftoverMessages(database, accountId());
}

QList<Message> ServiceRoot::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  return DatabaseQueries::getUndeletedMessagesForAccount(database, accountId());
}

void ServiceRoot::itemChanged(const QList<RootItem*> &items) {
  emit dataChanged(items);
}

void ServiceRoot::requestReloadMessageList(bool mark_selected_messages_read) {
  emit reloadMessageListRequested(mark_selected_messages_read);
}

void ServiceRoot::requestItemExpand(const QList<RootItem*> &items, bool expand) {
  emit itemExpandRequested(items, expand);
}

void ServiceRoot::requestItemExpandStateSave(RootItem *subtree_root) {
  emit itemExpandStateSaveRequested(subtree_root);
}

void ServiceRoot::requestItemReassignment(RootItem *item, RootItem *new_parent) {
  emit itemReassignmentRequested(item, new_parent);
}

void ServiceRoot::requestItemRemoval(RootItem *item) {
  emit itemRemovalRequested(item);
}

void ServiceRoot::syncIn() {
  QIcon original_icon = icon();

  setIcon(qApp->icons()->fromTheme(QSL("item-sync")));
  itemChanged(QList<RootItem*>() << this);

  RootItem *new_tree = obtainNewTreeForSyncIn();

  if (new_tree != NULL) {
    // Purge old data from SQL and clean all model items.
    requestItemExpandStateSave(this);
    removeOldFeedTree(false);
    cleanAllItems();

    // Model is clean, now store new tree into DB and
    // set primary IDs of the items.
    storeNewFeedTree(new_tree);

    // We have new feed, some feeds were maybe removed,
    // so remove left over messages.
    removeLeftOverMessages();

    foreach (RootItem *top_level_item, new_tree->childItems()) {
      top_level_item->setParent(NULL);
      requestItemReassignment(top_level_item, this);
    }

    updateCounts(true);

    new_tree->clearChildren();
    new_tree->deleteLater();

    QList<RootItem*> all_items = getSubTree();

    itemChanged(all_items);
    requestReloadMessageList(true);

    // Now we must refresh expand states.
    QList<RootItem*> items_to_expand;

    foreach (RootItem *item, all_items) {
      if (qApp->settings()->value(GROUP(CategoriesExpandStates), item->hashCode(), item->childCount() > 0).toBool()) {
        items_to_expand.append(item);
      }
    }

    items_to_expand.append(this);

    requestItemExpand(items_to_expand, true);
  }

  setIcon(original_icon);
  itemChanged(QList<RootItem*>() << this);
}

RootItem *ServiceRoot::obtainNewTreeForSyncIn() const {
  return NULL;
}

QStringList ServiceRoot::customIDSOfMessagesForItem(RootItem *item) {
  if (item->getParentServiceRoot() != this) {
    // Not item from this account.
    return QStringList();
  }
  else {
    QStringList list;

    switch (item->kind()) {
      case RootItemKind::Category: {
        foreach (RootItem *child, item->childItems()) {
          list.append(customIDSOfMessagesForItem(child));
        }

        return list;
      }

      case RootItemKind::ServiceRoot: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        list = DatabaseQueries::customIdsOfMessagesFromAccount(database, accountId());
        break;
      }

      case RootItemKind::Bin: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        list = DatabaseQueries::customIdsOfMessagesFromBin(database, accountId());
        break;
      }

      case RootItemKind::Feed: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        list = DatabaseQueries::customIdsOfMessagesFromFeed(database, item->customId(), accountId());
        break;
      }

      default:
        break;
    }

    return list;
  }
}

bool ServiceRoot::markFeedsReadUnread(QList<Feed*> items, RootItem::ReadStatus read) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::markFeedsReadUnread(database, textualFeedIds(items), accountId(), read)) {
    QList<RootItem*> itemss;

    foreach (Feed *feed, items) {
      feed->updateCounts(false);
      itemss.append(feed);
    }

    itemChanged(itemss);
    requestReloadMessageList(read == RootItem::Read);
    return true;
  }
  else {
    return false;
  }
}

QStringList ServiceRoot::textualFeedIds(const QList<Feed*> &feeds) const {
  QStringList stringy_ids;
  stringy_ids.reserve(feeds.size());

  foreach (const Feed *feed, feeds) {
    stringy_ids.append(QString("'%1'").arg(QString::number(feed->customId())));
  }

  return stringy_ids;
}

QStringList ServiceRoot::customIDsOfMessages(const QList<ImportanceChange> &changes) {
  QStringList list;

  for (int i = 0; i < changes.size(); i++) {
    list.append(changes.at(i).first.m_customId);
  }

  return list;
}

QStringList ServiceRoot::customIDsOfMessages(const QList<Message> &messages) {
  QStringList list;

  foreach (const Message &message, messages) {
    list.append(message.m_customId);
  }

  return list;
}

int ServiceRoot::accountId() const {
  return m_accountId;
}

void ServiceRoot::setAccountId(int account_id) {
  m_accountId = account_id;
}

bool ServiceRoot::loadMessagesForItem(RootItem *item, QSqlTableModel *model) {
  if (item->kind() == RootItemKind::Bin) {
    model->setFilter(QString("is_deleted = 1 AND is_pdeleted = 0 AND account_id = %1").arg(QString::number(accountId())));
  }
  else {
    QList<Feed*> children = item->getSubTreeFeeds();
    QString filter_clause = textualFeedIds(children).join(QSL(", "));

    model->setFilter(QString("feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = %2").arg(filter_clause,
                                                                                                            QString::number(accountId())));
    qDebug("Loading messages from feeds: %s.", qPrintable(filter_clause));
  }

  return true;
}

bool ServiceRoot::onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, RootItem::ReadStatus read) {
  Q_UNUSED(messages)
  Q_UNUSED(read)
  Q_UNUSED(selected_item)

  return true;
}

bool ServiceRoot::onAfterSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, RootItem::ReadStatus read) {
  Q_UNUSED(messages)
  Q_UNUSED(read)

  selected_item->updateCounts(false);
  itemChanged(QList<RootItem*>() << selected_item);
  return true;
}

bool ServiceRoot::onBeforeSwitchMessageImportance(RootItem *selected_item, const QList<ImportanceChange> &changes) {
  Q_UNUSED(selected_item)
  Q_UNUSED(changes)

  return true;
}

bool ServiceRoot::onAfterSwitchMessageImportance(RootItem *selected_item, const QList<ImportanceChange> &changes) {
  Q_UNUSED(selected_item)
  Q_UNUSED(changes)

  return true;
}

bool ServiceRoot::onBeforeMessagesDelete(RootItem *selected_item, const QList<Message> &messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)

  return true;
}

bool ServiceRoot::onAfterMessagesDelete(RootItem *selected_item, const QList<Message> &messages) {
  Q_UNUSED(messages)

  // User deleted some messages he selected in message list.
  selected_item->updateCounts(true);

  RecycleBin *bin = recycleBin();

  if (selected_item->kind() == RootItemKind::Bin) {
    itemChanged(QList<RootItem*>() << bin);
  }
  else {
    if (bin != NULL) {
      bin->updateCounts(true);
      itemChanged(QList<RootItem*>() << selected_item << bin);
    }
    else {
      itemChanged(QList<RootItem*>() << selected_item);
    }
  }

  return true;
}

bool ServiceRoot::onBeforeMessagesRestoredFromBin(RootItem *selected_item, const QList<Message> &messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)

  return true;
}

bool ServiceRoot::onAfterMessagesRestoredFromBin(RootItem *selected_item, const QList<Message> &messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)

  updateCounts(true);
  itemChanged(getSubTree());
  return true;
}

void ServiceRoot::assembleFeeds(Assignment feeds) {
  QHash<int,Category*> categories = getHashedSubTreeCategories();

  foreach (const AssignmentItem &feed, feeds) {
    if (feed.first == NO_PARENT_CATEGORY) {
      // This is top-level feed, add it to the root item.
      appendChild(feed.second);
    }
    else if (categories.contains(feed.first)) {
      // This feed belongs to this category.
      categories.value(feed.first)->appendChild(feed.second);
    }
    else {
      qWarning("Feed '%s' is loose, skipping it.", qPrintable(feed.second->title()));
    }
  }
}

void ServiceRoot::assembleCategories(Assignment categories) {
  QHash<int,RootItem*> assignments;
  assignments.insert(NO_PARENT_CATEGORY, this);

  // Add top-level categories.
  while (!categories.isEmpty()) {
    for (int i = 0; i < categories.size(); i++) {
      if (assignments.contains(categories.at(i).first)) {
        // Parent category of this category is already added.
        assignments.value(categories.at(i).first)->appendChild(categories.at(i).second);

        // Now, added category can be parent for another categories, add it.
        assignments.insert(categories.at(i).second->id(), categories.at(i).second);

        // Remove the category from the list, because it was
        // added to the final collection.
        categories.removeAt(i);
        i--;
      }
    }
  }
}
