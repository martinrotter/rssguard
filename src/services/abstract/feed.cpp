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

#include "services/abstract/feed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceroot.h"

#include <QSqlQuery>
#include <QSqlError>


Feed::Feed(RootItem *parent)
  : RootItem(parent), m_url(QString()), m_status(Normal), m_autoUpdateType(DefaultAutoUpdate),
    m_autoUpdateInitialInterval(DEFAULT_AUTO_UPDATE_INTERVAL), m_autoUpdateRemainingInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_totalCount(0), m_unreadCount(0) {
  setKind(RootItemKind::Feed);
}

Feed::~Feed() {
}

QList<Message> Feed::undeletedMessages() const {
  QList<Message> messages;
  int account_id = getParentServiceRoot()->accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);

  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT * "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;");

  query_read_msg.bindValue(QSL(":feed"), messageForeignKeyId());
  query_read_msg.bindValue(QSL(":account_id"), account_id);

  if (query_read_msg.exec()) {
    while (query_read_msg.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(query_read_msg.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }
  }

  return messages;
}

QVariant Feed::data(int column, int role) const {
  switch (role) {
    case Qt::ForegroundRole:
      switch (status()) {
        case NewMessages:
          return QColor(Qt::blue);

        case Error:
          return QColor(Qt::red);

        default:
          return QVariant();
      }

    default:
      return RootItem::data(column, role);
  }
}

int Feed::autoUpdateInitialInterval() const {
  return m_autoUpdateInitialInterval;
}

int Feed::countOfAllMessages() const {
  return m_totalCount;
}

int Feed::countOfUnreadMessages() const {
  return m_unreadCount;
}

void Feed::setCountOfAllMessages(int count_all_messages) {
  m_totalCount = count_all_messages;
}

void Feed::setCountOfUnreadMessages(int count_unread_messages) {
  m_unreadCount = count_unread_messages;
}

int Feed::update() {
  QList<Message> msgs = obtainNewMessages();
  return updateMessages(msgs);
}

void Feed::setAutoUpdateInitialInterval(int auto_update_interval) {
  // If new initial auto-update interval is set, then
  // we should reset time that remains to the next auto-update.
  m_autoUpdateInitialInterval = auto_update_interval;
  m_autoUpdateRemainingInterval = auto_update_interval;
}

Feed::AutoUpdateType Feed::autoUpdateType() const {
  return m_autoUpdateType;
}

void Feed::setAutoUpdateType(Feed::AutoUpdateType auto_update_type) {
  m_autoUpdateType = auto_update_type;
}

int Feed::autoUpdateRemainingInterval() const {
  return m_autoUpdateRemainingInterval;
}

void Feed::setAutoUpdateRemainingInterval(int auto_update_remaining_interval) {
  m_autoUpdateRemainingInterval = auto_update_remaining_interval;
}

void Feed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);

  query_all.setForwardOnly(true);

  if (including_total_count) {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
    query_all.bindValue(QSL(":feed"), customId());
    query_all.bindValue(QSL(":account_id"), getParentServiceRoot()->accountId());

    if (query_all.exec() && query_all.next()) {
      setCountOfAllMessages(query_all.value(0).toInt());
    }
  }

  query_all.prepare("SELECT count(*) FROM Messages "
                    "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 0 AND account_id = :account_id;");
  query_all.bindValue(QSL(":feed"), customId());
  query_all.bindValue(QSL(":account_id"), getParentServiceRoot()->accountId());

  // Obtain count of unread messages.
  if (query_all.exec() && query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (status() == NewMessages && new_unread_count < countOfUnreadMessages()) {
      setStatus(Normal);
    }

    setCountOfUnreadMessages(new_unread_count);
  }
}

int Feed::updateMessages(const QList<Message> &messages) {
  if (messages.isEmpty()) {
    return 0;
  }

  // Does not make any difference, since each feed now has
  // its own "custom ID" (standard feeds have their custom ID equal to primary key ID).
  int custom_id = customId();
  int account_id = getParentServiceRoot()->accountId();
  int updated_messages = 0;
  bool anything_updated = false;
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  // Prepare queries.
  QSqlQuery query_select_with_url(database);
  QSqlQuery query_select_with_id(database);
  QSqlQuery query_update(database);
  QSqlQuery query_insert(database);

  // Here we have query which will check for existence of the "same" message in given feed.
  // The two message are the "same" if:
  //   1) they belong to the same feed AND,
  //   2) they have same URL AND,
  //   3) they have same AUTHOR.
  query_select_with_url.setForwardOnly(true);
  query_select_with_url.prepare("SELECT id, date_created, is_read, is_important FROM Messages "
                                "WHERE feed = :feed AND url = :url AND author = :author AND account_id = :account_id;");

  // When we have custom ID of the message, we can check directly for existence
  // of that particular message.
  query_select_with_id.setForwardOnly(true);
  query_select_with_id.prepare("SELECT id, date_created, is_read, is_important FROM Messages "
                               "WHERE custom_id = :custom_id AND account_id = :account_id;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, is_read, is_important, url, author, date_created, contents, enclosures, custom_id, account_id) "
                       "VALUES (:feed, :title, :is_read, :is_important, :url, :author, :date_created, :contents, :enclosures, :custom_id, :account_id);");

  // Used to update existing messages.
  query_update.setForwardOnly(true);
  query_update.prepare("UPDATE Messages "
                       "SET title = :title, is_read = :is_read, is_important = :is_important, url = :url, author = :author, date_created = :date_created, contents = :contents, enclosures = :enclosures "
                       "WHERE id = :id;");

  if (!database.transaction()) {
    database.rollback();
    qDebug("Transaction start for message downloader failed: '%s'.", qPrintable(database.lastError().text()));
    return updated_messages;
  }

  foreach (Message message, messages) {
    // Check if messages contain relative URLs and if they do, then replace them.
    if (message.m_url.startsWith(QL1S("/"))) {
      QString new_message_url = QUrl(url()).toString(QUrl::RemoveUserInfo |
                                                     QUrl::RemovePath |
                                                     QUrl::RemoveQuery |
                                               #if QT_VERSION >= 0x050000
                                                     QUrl::RemoveFilename |
                                               #endif
                                                     QUrl::StripTrailingSlash);

      new_message_url += message.m_url;
      message.m_url = new_message_url;
    }

    int id_existing_message = -1;
    qint64 date_existing_message;
    bool is_read_existing_message;
    bool is_important_existing_message;

    if (message.m_customId.isEmpty()) {
      // We need to recognize existing messages according URL & AUTHOR.
      // NOTE: This concerns messages from standard account.
      query_select_with_url.bindValue(QSL(":feed"), custom_id);
      query_select_with_url.bindValue(QSL(":url"), message.m_url);
      query_select_with_url.bindValue(QSL(":author"), message.m_author);
      query_select_with_url.bindValue(QSL(":account_id"), account_id);

      if (query_select_with_url.exec() && query_select_with_url.next()) {
        id_existing_message = query_select_with_url.value(0).toInt();
        date_existing_message = query_select_with_url.value(1).value<qint64>();
        is_read_existing_message = query_select_with_url.value(2).toBool();
        is_important_existing_message = query_select_with_url.value(3).toBool();
      }

      query_select_with_url.finish();
    }
    else {
      // We can recognize existing messages via their custom ID.
      // NOTE: This concerns messages from custom accounts, like TT-RSS or ownCloud News.
      query_select_with_id.bindValue(QSL(":account_id"), account_id);
      query_select_with_id.bindValue(QSL(":custom_id"), message.m_customId);

      if (query_select_with_id.exec() && query_select_with_id.next()) {
        id_existing_message = query_select_with_id.value(0).toInt();
        date_existing_message = query_select_with_id.value(1).value<qint64>();
        is_read_existing_message = query_select_with_id.value(2).toBool();
        is_important_existing_message = query_select_with_id.value(3).toBool();
      }

      query_select_with_id.finish();
    }

    // Now, check if this message is already in the DB.
    if (id_existing_message >= 0) {
      // Message is already in the DB.
      //
      // Now, we update it if at least one of next conditions is true:
      //   1) Message has custom ID AND (its date OR read status OR starred status are changed).
      //   2) Message has its date fetched from feed AND its date is different from date in DB.
      if (/* 1 */ (!message.m_customId.isEmpty() && (message.m_created.toMSecsSinceEpoch() != date_existing_message || message.m_isRead != is_read_existing_message || message.m_isImportant != is_important_existing_message)) ||
          /* 2 */ (message.m_createdFromFeed && message.m_created.toMSecsSinceEpoch() != date_existing_message)) {
        // Message exists, it is changed, update it.
        query_update.bindValue(QSL(":title"), message.m_title);
        query_update.bindValue(QSL(":is_read"), (int) message.m_isRead);
        query_update.bindValue(QSL(":is_important"), (int) message.m_isImportant);
        query_update.bindValue(QSL(":url"), message.m_url);
        query_update.bindValue(QSL(":author"), message.m_author);
        query_update.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
        query_update.bindValue(QSL(":contents"), message.m_contents);
        query_update.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
        query_update.bindValue(QSL(":id"), id_existing_message);

        anything_updated = true;

        if (query_update.exec()) {
          updated_messages++;
        }

        query_update.finish();
        qDebug("Updating message '%s' in DB.", qPrintable(message.m_title));
      }
    }
    else {
      // Message with this URL is not fetched in this feed yet.
      query_insert.bindValue(QSL(":feed"), custom_id);
      query_insert.bindValue(QSL(":title"), message.m_title);
      query_insert.bindValue(QSL(":is_read"), (int) message.m_isRead);
      query_insert.bindValue(QSL(":is_important"), (int) message.m_isImportant);
      query_insert.bindValue(QSL(":url"), message.m_url);
      query_insert.bindValue(QSL(":author"), message.m_author);
      query_insert.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
      query_insert.bindValue(QSL(":contents"), message.m_contents);
      query_insert.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
      query_insert.bindValue(QSL(":custom_id"), message.m_customId);
      query_insert.bindValue(QSL(":account_id"), account_id);

      if (query_insert.exec() && query_insert.numRowsAffected() == 1) {
        updated_messages++;
      }

      query_insert.finish();
      qDebug("Adding new message '%s' to DB.", qPrintable(message.m_title));
    }
  }

  // Now, fixup custom IDS for messages which initially did not have them,
  // just to keep the data consistent.
  if (database.exec("UPDATE Messages "
                    "SET custom_id = (SELECT id FROM Messages t WHERE t.id = Messages.id) "
                    "WHERE Messages.custom_id IS NULL OR Messages.custom_id = '';").lastError().isValid()) {
    qWarning("Failed to set custom ID for all messages.");
  }

  if (!database.commit()) {
    database.rollback();
    qDebug("Transaction commit for message downloader failed.");
  }
  else {
    if (updated_messages > 0) {
      setStatus(NewMessages);
    }
    else {
      setStatus(Normal);
    }

    QList<RootItem*> items_to_update;

    updateCounts(true);
    items_to_update.append(this);

    if (getParentServiceRoot()->recycleBin() != NULL && anything_updated) {
      getParentServiceRoot()->recycleBin()->updateCounts(true);
      items_to_update.append(getParentServiceRoot()->recycleBin());
    }

    getParentServiceRoot()->itemChanged(items_to_update);
  }

  return updated_messages;
}
