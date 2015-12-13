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

#include "services/abstract/recyclebin.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/serviceroot.h"

#include <QSqlQuery>


RecycleBin::RecycleBin(RootItem *parent_item) : RootItem(parent_item) {
  setKind(RootItemKind::Bin);
  setIcon(qApp->icons()->fromTheme(QSL("folder-recycle-bin")));
  setTitle(tr("Recycle bin"));
  setDescription(tr("Recycle bin contains all deleted messages from all feeds."));
  setCreationDate(QDateTime::currentDateTime());
}

RecycleBin::~RecycleBin() {
}

int RecycleBin::countOfUnreadMessages() const {
  return m_unreadCount;
}

int RecycleBin::countOfAllMessages() const {
  return m_totalCount;
}

void RecycleBin::updateCounts(bool update_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);
  ServiceRoot *parent_root = getParentServiceRoot();

  query_all.setForwardOnly(true);
  query_all.prepare("SELECT count(*) FROM Messages "
                    "WHERE is_read = 0 AND is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  query_all.bindValue(QSL(":account_id"), parent_root->accountId());


  if (query_all.exec() && query_all.next()) {
    m_unreadCount = query_all.value(0).toInt();
  }
  else {
    m_unreadCount = 0;
  }

  if (update_total_count) {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
    query_all.bindValue(QSL(":account_id"), parent_root->accountId());

    if (query_all.exec() && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
    else {
      m_totalCount = 0;
    }
  }
}

QVariant RecycleBin::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      return tr("Recycle bin\n\n%1").arg(tr("%n deleted message(s).", 0, countOfAllMessages()));

    default:
      return RootItem::data(column, role);
  }
}

QList<Message> RecycleBin::undeletedMessages() const {
  QList<Message> messages;
  int account_id = const_cast<RecycleBin*>(this)->getParentServiceRoot()->accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);

  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT * "
                         "FROM Messages "
                         "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  query_read_msg.bindValue(QSL(":account_id"), account_id);

  // FIXME: Fix those const functions, this is fucking ugly.

  if (query_read_msg.exec()) {
    while (query_read_msg.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(query_read_msg.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }

      messages.append(message);
    }
  }

  return messages;
}

bool RecycleBin::markAsReadUnread(RootItem::ReadStatus status) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin read change.");
    return false;
  }

  QSqlQuery query_read_msg(db_handle);
  ServiceRoot *parent_root = getParentServiceRoot();

  query_read_msg.setForwardOnly(true);

  if (!query_read_msg.prepare("UPDATE Messages SET is_read = :read "
                              "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;")) {
    qWarning("Query preparation failed for recycle bin read change.");

    db_handle.rollback();
    return false;
  }

  query_read_msg.bindValue(QSL(":read"), status == RootItem::Read ? 1 : 0);
  query_read_msg.bindValue(QSL(":account_id"), parent_root->accountId());

  if (!query_read_msg.exec()) {
    qDebug("Query execution for recycle bin read change failed.");
    db_handle.rollback();
  }

  // Commit changes.
  if (db_handle.commit()) {
    updateCounts(false);

    parent_root->itemChanged(QList<RootItem*>() << this);
    parent_root->requestReloadMessageList(status == RootItem::Read);
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool RecycleBin::cleanMessages(bool clear_only_read) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin emptying.");
    return false;
  }

  ServiceRoot *parent_root = getParentServiceRoot();
  QSqlQuery query_empty_bin(db_handle);

  query_empty_bin.setForwardOnly(true);

  if (clear_only_read) {
    query_empty_bin.prepare("UPDATE Messages SET is_pdeleted = 1 "
                            "WHERE is_read = 1 AND is_deleted = 1 AND account_id = :account_id;");
  }
  else {
    query_empty_bin.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_deleted = 1 AND account_id = :account_id;"));
  }

  query_empty_bin.bindValue(QSL(":account_id"), parent_root->accountId());

  if (!query_empty_bin.exec()) {
    qWarning("Query execution failed for recycle bin emptying.");

    db_handle.rollback();
    return false;
  }

  // Commit changes.
  if (db_handle.commit()) {
    updateCounts(true);
    parent_root->itemChanged(QList<RootItem*>() << this);
    parent_root->requestReloadMessageList(true);
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool RecycleBin::empty() {
  return cleanMessages(false);
}

bool RecycleBin::restore() {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin restoring.");
    return false;
  }

  ServiceRoot *parent_root = getParentServiceRoot();
  QSqlQuery query_empty_bin(db_handle);

  query_empty_bin.setForwardOnly(true);
  query_empty_bin.prepare("UPDATE Messages SET is_deleted = 0 "
                          "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  query_empty_bin.bindValue(QSL(":account_id"), parent_root->accountId());

  if (!query_empty_bin.exec()) {
    qWarning("Query execution failed for recycle bin restoring.");

    db_handle.rollback();
    return false;
  }

  // Commit changes.
  if (db_handle.commit()) {
    parent_root->updateCounts(true);
    parent_root->itemChanged(parent_root->getSubTree());
    parent_root->requestReloadMessageList(true);
    parent_root->requestFeedReadFilterReload();
    return true;
  }
  else {
    return db_handle.rollback();
  }
}
