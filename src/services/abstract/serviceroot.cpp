// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "miscellaneous/textfactory.h"
#include "services/abstract/category.h"

#include <QSqlQuery>


ServiceRoot::ServiceRoot(RootItem *parent) : RootItem(parent), m_accountId(NO_PARENT_CATEGORY) {
  setKind(RootItemKind::ServiceRoot);
}

ServiceRoot::~ServiceRoot() {
}

bool ServiceRoot::deleteViaGui() {
  QSqlDatabase connection = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  // Remove all messages.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM Messages WHERE account_id = %1;").arg(accountId()))) {
    return false;
  }

  // Remove all feeds.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM Feeds WHERE account_id = %1;").arg(accountId()))) {
    return false;
  }

  // Remove all categories.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM Categories WHERE account_id = %1;").arg(accountId()))) {
    return false;
  }

  // Switch "existence" flag.
  bool data_removed = QSqlQuery(connection).exec(QString("DELETE FROM Accounts WHERE id = %1;").arg(accountId()));

  if (data_removed) {
    requestItemRemoval(this);
  }

  return data_removed;
}

QList<Message> ServiceRoot::undeletedMessages() const {
  QList<Message> messages;
  int account_id = accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);

  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT title, url, author, date_created, contents, enclosures, custom_id, id, feed "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  query_read_msg.bindValue(QSL(":account_id"), account_id);

  // FIXME: Fix those const functions, this is fucking ugly.

  if (query_read_msg.exec()) {
    while (query_read_msg.next()) {
      Message message;

      // TODO: napsat funkci static Message Message::fromSqlRecord(const QSqlRecord &record)
      // ta prostě bude brat record z SELECT * FROM Messages WHERE ....;
      // a vrati ho jako objekt Message;

      message.m_feedId = query_read_msg.value(7).toString();
      message.m_title = query_read_msg.value(0).toString();
      message.m_url = query_read_msg.value(1).toString();
      message.m_author = query_read_msg.value(2).toString();
      message.m_created = TextFactory::parseDateTime(query_read_msg.value(3).value<qint64>());
      message.m_contents = query_read_msg.value(4).toString();
      message.m_enclosures = Enclosures::decodeEnclosuresFromString(query_read_msg.value(5).toString());
      message.m_accountId = account_id;
      message.m_customId = query_read_msg.value(6).toString();
      message.m_id = query_read_msg.value(7).toInt();

      messages.append(message);
    }
  }

  return messages;
}

void ServiceRoot::itemChanged(const QList<RootItem *> &items) {
  emit dataChanged(items);
}

void ServiceRoot::requestReloadMessageList(bool mark_selected_messages_read) {
  emit reloadMessageListRequested(mark_selected_messages_read);
}

void ServiceRoot::requestFeedReadFilterReload() {
  emit readFeedsFilterInvalidationRequested();
}

void ServiceRoot::requestItemExpand(const QList<RootItem *> &items, bool expand) {
  emit itemExpandRequested(items, expand);
}

void ServiceRoot::requestItemReassignment(RootItem *item, RootItem *new_parent) {
  emit itemReassignmentRequested(item, new_parent);
}

void ServiceRoot::requestItemRemoval(RootItem *item) {
  emit itemRemovalRequested(item);
}

int ServiceRoot::accountId() const {
  return m_accountId;
}

void ServiceRoot::setAccountId(int account_id) {
  m_accountId = account_id;
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
