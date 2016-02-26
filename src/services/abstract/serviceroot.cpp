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
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/recyclebin.h"

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>


ServiceRoot::ServiceRoot(RootItem *parent) : RootItem(parent), m_accountId(NO_PARENT_CATEGORY) {
  setKind(RootItemKind::ServiceRoot);
  setCreationDate(QDateTime::currentDateTime());
}

ServiceRoot::~ServiceRoot() {
}

bool ServiceRoot::deleteViaGui() {
  QSqlDatabase connection = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(connection);
  const int account_id = accountId();
  query.setForwardOnly(true);

  QStringList queries;
  queries << QSL("DELETE FROM Messages WHERE account_id = :account_id;") <<
             QSL("DELETE FROM Feeds WHERE account_id = :account_id;") <<
             QSL("DELETE FROM Categories WHERE account_id = :account_id;") <<
             QSL("DELETE FROM Accounts WHERE id = :account_id;");

  foreach (const QString &q, queries) {
    query.prepare(q);
    query.bindValue(QSL(":account_id"), account_id);

    if (!query.exec()) {
      qCritical("Removing of account from DB failed, this is critical: '%s'.", qPrintable(query.lastError().text()));
      return false;
    }
    else {
      query.finish();
    }
  }

  requestItemRemoval(this);
  return true;
}

bool ServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(db_handle);
  query.setForwardOnly(true);
  query.prepare(QSL("UPDATE Messages SET is_read = :read WHERE is_pdeleted = 0 AND account_id = :account_id;"));

  query.bindValue(QSL(":account_id"), accountId());
  query.bindValue(QSL(":read"), status == RootItem::Read ? 1 : 0);

  if (query.exec()) {
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
  QSqlQuery query(database);
  query.setForwardOnly(true);

  query.prepare(QSL("DELETE FROM Feeds WHERE account_id = :account_id;"));
  query.bindValue(QSL(":account_id"), accountId());
  query.exec();

  query.prepare(QSL("DELETE FROM Categories WHERE account_id = :account_id;"));
  query.bindValue(QSL(":account_id"), accountId());
  query.exec();

  if (including_messages) {
    query.prepare(QSL("DELETE FROM Messages WHERE account_id = :account_id;"));
    query.bindValue(QSL(":account_id"), accountId());
    query.exec();
  }
}

void ServiceRoot::cleanAllItems() {
  foreach (RootItem *top_level_item, childItems()) {
    if (top_level_item->kind() != RootItemKind::Bin) {
      requestItemRemoval(top_level_item);
    }
  }
}

bool ServiceRoot::cleanFeeds(QList<Feed *> items, bool clean_read_only) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_delete_msg(db_handle);
  int account_id = accountId();
  query_delete_msg.setForwardOnly(true);

  if (clean_read_only) {
    query_delete_msg.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                                     "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 1 AND account_id = :account_id;")
                             .arg(textualFeedIds(items).join(QSL(", "))));
  }
  else {
    query_delete_msg.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                                     "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;")
                             .arg(textualFeedIds(items).join(QSL(", "))));
  }

  query_delete_msg.bindValue(QSL(":deleted"), 1);
  query_delete_msg.bindValue(QSL(":account_id"), account_id);

  if (query_delete_msg.exec()) {
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
    qDebug("Cleaning of feeds failed: '%s'.", qPrintable(query_delete_msg.lastError().text()));
    return false;
  }
}

void ServiceRoot::storeNewFeedTree(RootItem *root) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_category(database);
  QSqlQuery query_feed(database);

  query_category.prepare("INSERT INTO Categories (parent_id, title, account_id, custom_id) "
                         "VALUES (:parent_id, :title, :account_id, :custom_id);");
  query_feed.prepare("INSERT INTO Feeds (title, icon, category, protected, update_type, update_interval, account_id, custom_id) "
                     "VALUES (:title, :icon, :category, :protected, :update_type, :update_interval, :account_id, :custom_id);");

  // Iterate all children.
  foreach (RootItem *child, root->getSubTree()) {
    if (child->kind() == RootItemKind::Category) {
      query_category.bindValue(QSL(":parent_id"), child->parent()->id());
      query_category.bindValue(QSL(":title"), child->title());
      query_category.bindValue(QSL(":account_id"), accountId());
      query_category.bindValue(QSL(":custom_id"), QString::number(child->toCategory()->customId()));

      if (query_category.exec()) {
        child->setId(query_category.lastInsertId().toInt());
      }
    }
    else if (child->kind() == RootItemKind::Feed) {
      Feed *feed = child->toFeed();

      query_feed.bindValue(QSL(":title"), feed->title());
      query_feed.bindValue(QSL(":icon"), qApp->icons()->toByteArray(feed->icon()));
      query_feed.bindValue(QSL(":category"), feed->parent()->id());
      query_feed.bindValue(QSL(":protected"), 0);
      query_feed.bindValue(QSL(":update_type"), (int) feed->autoUpdateType());
      query_feed.bindValue(QSL(":update_interval"), feed->autoUpdateInitialInterval());
      query_feed.bindValue(QSL(":account_id"), accountId());
      query_feed.bindValue(QSL(":custom_id"), feed->customId());

      if (query_feed.exec()) {
        feed->setId(query_feed.lastInsertId().toInt());
      }
    }
  }

  RecycleBin *bin = recycleBin();

  if (bin != NULL && !childItems().contains(bin)) {
    // As the last item, add recycle bin, which is needed.
    appendChild(bin);
    bin->updateCounts(true);
  }
}

void ServiceRoot::removeLeftOverMessages() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(database);
  int account_id = accountId();

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE account_id = :account_id AND feed NOT IN (SELECT custom_id FROM Feeds WHERE account_id = :account_id);"));
  query.bindValue(QSL(":account_id"), account_id);

  if (!query.exec()) {
    qWarning("Removing of left over messages failed: '%s'.", qPrintable(query.lastError().text()));
  }
}

QList<Message> ServiceRoot::undeletedMessages() const {
  QList<Message> messages;
  const int account_id = accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(database);

  query.setForwardOnly(true);
  query.prepare("SELECT * "
                "FROM Messages "
                "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  query.bindValue(QSL(":account_id"), account_id);

  if (query.exec()) {
    while (query.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(query.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }

      messages.append(message);
    }
  }

  return messages;
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
        QSqlQuery query(database);

        query.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;"));
        query.bindValue(QSL(":account_id"), accountId());
        query.exec();

        while (query.next()) {
          list.append(query.value(0).toString());
        }

        break;
      }

      case RootItemKind::Bin: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        QSqlQuery query(database);

        query.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;"));
        query.bindValue(QSL(":account_id"), accountId());
        query.exec();

        while (query.next()) {
          list.append(query.value(0).toString());
        }

        break;
      }

      case RootItemKind::Feed: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        QSqlQuery query(database);

        query.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;"));
        query.bindValue(QSL(":account_id"), accountId());
        query.bindValue(QSL(":feed"), item->customId());
        query.exec();

        while (query.next()) {
          list.append(query.value(0).toString());
        }

        break;
      }

      default:
        break;
    }

    return list;
  }
}

bool ServiceRoot::markFeedsReadUnread(QList<Feed*> items, RootItem::ReadStatus read) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(db_handle);
  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare(QString("UPDATE Messages SET is_read = :read "
                                 "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;").arg(textualFeedIds(items).join(QSL(", "))));

  query_read_msg.bindValue(QSL(":read"), read == RootItem::Read ? 1 : 0);
  query_read_msg.bindValue(QSL(":account_id"), accountId());

  if (query_read_msg.exec()) {
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
    stringy_ids.append(QString("'%1'").arg(QString::number(feed->messageForeignKeyId())));
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
      feed.second->updateCounts(true);
    }
    else if (categories.contains(feed.first)) {
      // This feed belongs to this category.
      categories.value(feed.first)->appendChild(feed.second);
      feed.second->updateCounts(true);
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
