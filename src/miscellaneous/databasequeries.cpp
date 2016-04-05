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

#include "miscellaneous/databasequeries.h"

#include <QVariant>
#include <QUrl>
#include <QSqlError>


bool DatabaseQueries::markMessagesRead(QSqlDatabase db, const QStringList &ids, RootItem::ReadStatus read) {
  QSqlQuery query_read_msg(db);
  query_read_msg.setForwardOnly(true);

  return query_read_msg.exec(QString(QSL("UPDATE Messages SET is_read = %2 WHERE id IN (%1);"))
                             .arg(ids.join(QSL(", ")), read == RootItem::Read ? QSL("1") : QSL("0")));
}

bool DatabaseQueries::markMessageImportant(QSqlDatabase db, int id, RootItem::Importance importance) {
  QSqlQuery q;(db);
  q.setForwardOnly(true);

  if (!q.prepare(QSL("UPDATE Messages SET is_important = :important WHERE id = :id;"))) {
    qWarning("Query preparation failed for message importance switch.");
    return false;
  }

  q.bindValue(QSL(":id"), id);
  q.bindValue(QSL(":important"), (int) importance);

  // Commit changes.
  return q.exec();
}

bool DatabaseQueries::switchMessagesImportance(QSqlDatabase db, const QStringList &ids) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  return q.exec(QString(QSL("UPDATE Messages SET is_important = NOT is_important WHERE id IN (%1);")).arg(ids.join(QSL(", "))));
}

bool DatabaseQueries::permanentlyDeleteMessages(QSqlDatabase db, const QStringList &ids) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  return q.exec(QString(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE id IN (%1);")).arg(ids.join(QSL(", "))));
}

bool DatabaseQueries::deleteOrRestoreMessagesToFromBin(QSqlDatabase db, const QStringList &ids, bool deleted) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  return q.exec(QString(QSL("UPDATE Messages SET is_deleted = %2 WHERE id IN (%1);")).arg(ids.join(QSL(", ")),
                                                                                          QString::number(deleted ? 1 : 0)));
}

bool DatabaseQueries::purgeImportantMessages(QSqlDatabase db) {
  QSqlQuery query = QSqlQuery(db);
  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = 1;"));

  return query.exec();
}

bool DatabaseQueries::purgeReadMessages(QSqlDatabase db) {
  QSqlQuery query = QSqlQuery(db);
  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND is_deleted = :is_deleted AND is_read = :is_read;"));
  query.bindValue(QSL(":is_read"), 1);

  // Remove only messages which are NOT in recycle bin.
  query.bindValue(QSL(":is_deleted"), 0);

  // Remove only messages which are NOT starred.
  query.bindValue(QSL(":is_important"), 0);

  return query.exec();
}

bool DatabaseQueries::purgeOldMessages(QSqlDatabase db, int older_than_days) {
  QSqlQuery query = QSqlQuery(db);
  const qint64 since_epoch = QDateTime::currentDateTimeUtc().addDays(-older_than_days).toMSecsSinceEpoch();

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND date_created < :date_created;"));
  query.bindValue(QSL(":date_created"), since_epoch);

  // Remove only messages which are NOT starred.
  query.bindValue(QSL(":is_important"), 0);

  return query.exec();
}

bool DatabaseQueries::purgeRecycleBin(QSqlDatabase db) {
  QSqlQuery query = QSqlQuery(db);

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND is_deleted = :is_deleted;"));
  query.bindValue(QSL(":is_deleted"), 1);

  // Remove only messages which are NOT starred.
  query.bindValue(QSL(":is_important"), 0);

  return query.exec();
}

QMap<int,int> DatabaseQueries::getMessageCountsForCategory(QSqlDatabase db, int custom_id, int account_id,
                                                           bool including_total_counts, bool *ok) {
  QMap<int,int> counts;
  QSqlQuery q(db);
  q.setForwardOnly(true);

  if (including_total_counts) {
    q.prepare("SELECT feed, count(*) FROM Messages "
              "WHERE feed IN (SELECT custom_id FROM Feeds WHERE category = :category AND account_id = :account_id) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
              "GROUP BY feed;");
  }
  else {
    q.prepare("SELECT feed, count(*) FROM Messages "
              "WHERE feed IN (SELECT custom_id FROM Feeds WHERE category = :category AND account_id = :account_id) AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 0 AND account_id = :account_id "
              "GROUP BY feed;");
  }

  q.bindValue(QSL(":category"), custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      int feed_id = q.value(0).toInt();
      int new_count = q.value(1).toInt();

      counts.insert(feed_id, new_count);
    }

    if (ok != NULL) {
      *ok = true;
    }
  }
  else {
    if (ok != NULL) {
      *ok = false;
    }
  }

  return counts;
}

int DatabaseQueries::getMessageCountsForFeed(QSqlDatabase db, int feed_custom_id,
                                             int account_id, bool including_total_counts, bool *ok) {
  QSqlQuery query_all(db);
  query_all.setForwardOnly(true);

  if (including_total_counts) {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  }
  else {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 0 AND account_id = :account_id;");
  }

  query_all.bindValue(QSL(":feed"), feed_custom_id);
  query_all.bindValue(QSL(":account_id"), account_id);

  if (query_all.exec() && query_all.next()) {
    if (ok != NULL) {
      *ok = true;
    }

    return query_all.value(0).toInt();
  }
  else {
    if (ok != NULL) {
      *ok = false;
    }

    return 0;
  }
}

int DatabaseQueries::getMessageCountsForBin(QSqlDatabase db, int account_id, bool including_total_counts, bool *ok) {
  QSqlQuery query_all(db);
  query_all.setForwardOnly(true);

  if (including_total_counts) {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  }
  else {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE is_read = 0 AND is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  }

  query_all.bindValue(QSL(":account_id"), account_id);

  if (query_all.exec() && query_all.next()) {
    if (ok != NULL) {
      *ok = true;
    }

    return query_all.value(0).toInt();
  }
  else {
    if (ok != NULL) {
      *ok = false;
    }

    return 0;
  }
}

QList<Message> DatabaseQueries::getUndeletedMessages(QSqlDatabase db, int feed_custom_id, int account_id, bool *ok) {
  QList<Message> messages;
  QSqlQuery q(db);
  q.setForwardOnly(true);
  q.prepare("SELECT * "
            "FROM Messages "
            "WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;");

  q.bindValue(QSL(":feed"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != NULL) {
      *ok = true;
    }
  }
  else {
    if (ok != NULL) {
      *ok = false;
    }
  }

  return messages;
}

int DatabaseQueries::updateMessages(QSqlDatabase db,
                                    const QList<Message> &messages,
                                    int feed_custom_id,
                                    int account_id,
                                    const QString &url,
                                    bool *any_message_changed,
                                    bool *ok) {
  if (messages.isEmpty()) {
    *any_message_changed = false;
    *ok = true;
    return 0;
  }

  // Does not make any difference, since each feed now has
  // its own "custom ID" (standard feeds have their custom ID equal to primary key ID).
  int updated_messages = 0;

  // Prepare queries.
  QSqlQuery query_select_with_url(db);
  QSqlQuery query_select_with_id(db);
  QSqlQuery query_update(db);
  QSqlQuery query_insert(db);

  // Here we have query which will check for existence of the "same" message in given feed.
  // The two message are the "same" if:
  //   1) they belong to the same feed AND,
  //   2) they have same URL AND,
  //   3) they have same AUTHOR.
  query_select_with_url.setForwardOnly(true);
  query_select_with_url.prepare("SELECT id, date_created, is_read, is_important FROM Messages "
                                "WHERE feed = :feed AND title = :title AND url = :url AND author = :author AND account_id = :account_id;");

  // When we have custom ID of the message, we can check directly for existence
  // of that particular message.
  query_select_with_id.setForwardOnly(true);
  query_select_with_id.prepare("SELECT id, date_created, is_read, is_important FROM Messages "
                               "WHERE custom_id = :custom_id AND account_id = :account_id;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, is_read, is_important, url, author, date_created, contents, enclosures, custom_id, custom_hash, account_id) "
                       "VALUES (:feed, :title, :is_read, :is_important, :url, :author, :date_created, :contents, :enclosures, :custom_id, :custom_hash, :account_id);");

  // Used to update existing messages.
  query_update.setForwardOnly(true);
  query_update.prepare("UPDATE Messages "
                       "SET title = :title, is_read = :is_read, is_important = :is_important, url = :url, author = :author, date_created = :date_created, contents = :contents, enclosures = :enclosures "
                       "WHERE id = :id;");

  if (!db.transaction()) {
    db.rollback();
    qDebug("Transaction start for message downloader failed: '%s'.", qPrintable(db.lastError().text()));
    return updated_messages;
  }

  foreach (Message message, messages) {
    // Check if messages contain relative URLs and if they do, then replace them.
    if (message.m_url.startsWith(QL1S("/"))) {
      QString new_message_url = QUrl(url).toString(QUrl::RemoveUserInfo |
                                                   QUrl::RemovePath |
                                                   QUrl::RemoveQuery |
                                                   QUrl::RemoveFilename |
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
      query_select_with_url.bindValue(QSL(":feed"), feed_custom_id);
      query_select_with_url.bindValue(QSL(":title"), message.m_title);
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

        *any_message_changed = true;

        if (query_update.exec() && !message.m_isRead) {
          updated_messages++;
        }

        query_update.finish();
        qDebug("Updating message '%s' in DB.", qPrintable(message.m_title));
      }
    }
    else {
      // Message with this URL is not fetched in this feed yet.
      query_insert.bindValue(QSL(":feed"), feed_custom_id);
      query_insert.bindValue(QSL(":title"), message.m_title);
      query_insert.bindValue(QSL(":is_read"), (int) message.m_isRead);
      query_insert.bindValue(QSL(":is_important"), (int) message.m_isImportant);
      query_insert.bindValue(QSL(":url"), message.m_url);
      query_insert.bindValue(QSL(":author"), message.m_author);
      query_insert.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
      query_insert.bindValue(QSL(":contents"), message.m_contents);
      query_insert.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
      query_insert.bindValue(QSL(":custom_id"), message.m_customId);
      query_insert.bindValue(QSL(":custom_hash"), message.m_customHash);
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
  if (db.exec("UPDATE Messages "
              "SET custom_id = (SELECT id FROM Messages t WHERE t.id = Messages.id) "
              "WHERE Messages.custom_id IS NULL OR Messages.custom_id = '';").lastError().isValid()) {
    qWarning("Failed to set custom ID for all messages.");
  }

  if (!db.commit()) {
    db.rollback();
    qDebug("Transaction commit for message downloader failed.");

    if (ok != NULL) {
      *ok = false;
    }
  }
  else {
    if (ok != NULL) {
      *ok = true;
    }
  }

  return updated_messages;
}

DatabaseQueries::DatabaseQueries() {
}
