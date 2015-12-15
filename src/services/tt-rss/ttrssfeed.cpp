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

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include <QSqlQuery>
#include <QSqlError>


TtRssFeed::TtRssFeed(RootItem *parent)
  : Feed(parent), m_customId(NO_PARENT_CATEGORY), m_totalCount(0), m_unreadCount(0) {
}

TtRssFeed::TtRssFeed(const QSqlRecord &record) : Feed(NULL), m_totalCount(0), m_unreadCount(0) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
}

TtRssFeed::~TtRssFeed() {
}

TtRssServiceRoot *TtRssFeed::serviceRoot() {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

void TtRssFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);

  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = '%1' AND is_deleted = 0 AND account_id = %2;").arg(QString::number(customId()),
                                                                                                                              QString::number(serviceRoot()->accountId()))) && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = '%1' AND is_deleted = 0 AND is_read = 0 AND account_id = %2;").arg(QString::number(customId()),
                                                                                                                                            QString::number(serviceRoot()->accountId()))) && query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (status() == NewMessages && new_unread_count < m_unreadCount) {
      setStatus(Normal);
    }

    m_unreadCount = new_unread_count;
  }
}

int TtRssFeed::countOfAllMessages() const {
  return m_totalCount;
}

int TtRssFeed::countOfUnreadMessages() const {
  return m_unreadCount;
}

int TtRssFeed::update() {
  QList<Message> messages;
  int newly_added_messages = 0;
  int limit = MAX_MESSAGES;
  int skip = 0;

  do {
    TtRssGetHeadlinesResponse headlines = serviceRoot()->network()->getHeadlines(customId(), limit, skip,
                                                                                 true, true, false);

    if (serviceRoot()->network()->lastError() != QNetworkReply::NoError) {
      setStatus(Feed::Error);
      serviceRoot()->itemChanged(QList<RootItem*>() << this);
      return 0;
    }
    else {
      QList<Message> new_messages = headlines.messages();

      messages.append(new_messages);
      newly_added_messages = new_messages.size();
      skip += newly_added_messages;
    }
  }
  while (newly_added_messages > 0);

  return updateMessages(messages);
}

QList<Message> TtRssFeed::undeletedMessages() const {
  QList<Message> messages;
  int account_id = const_cast<TtRssFeed*>(this)->serviceRoot()->accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);

  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT * "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;");

  query_read_msg.bindValue(QSL(":feed"), customId());
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

bool TtRssFeed::markAsReadUnread(RootItem::ReadStatus status) {
  QStringList ids = serviceRoot()->customIDSOfMessagesForItem(this);
  TtRssUpdateArticleResponse response = serviceRoot()->network()->updateArticles(ids, UpdateArticle::Unread,
                                                                                 status == RootItem::Unread ?
                                                                                   UpdateArticle::SetToTrue :
                                                                                   UpdateArticle::SetToFalse);

  if (serviceRoot()->network()->lastError() != QNetworkReply::NoError || response.updateStatus()  != STATUS_OK) {
    return false;
  }
  else {
    return serviceRoot()->markFeedsReadUnread(QList<Feed*>() << this, status);
  }
}

bool TtRssFeed::cleanMessages(bool clear_only_read) {
  return serviceRoot()->cleanFeeds(QList<Feed*>() << this, clear_only_read);
}

int TtRssFeed::customId() const {
  return m_customId;
}

void TtRssFeed::setCustomId(int custom_id) {
  m_customId = custom_id;
}

int TtRssFeed::updateMessages(const QList<Message> &messages) {
  if (messages.isEmpty()) {
    return 0;
  }

  int feed_id = customId();
  int updated_messages = 0;
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  int account_id = serviceRoot()->accountId();

  // Prepare queries.
  QSqlQuery query_insert(database);
  QSqlQuery query_select(database);
  QSqlQuery query_update(database);

  query_update.setForwardOnly(true);
  query_update.prepare("UPDATE Messages "
                       "SET title = :title, is_read = :is_read, is_important = :is_important, url = :url, author = :author, date_created = :date_created, contents = :contents, enclosures = :enclosures "
                       "WHERE id = :id;");

  query_select.setForwardOnly(true);
  query_select.prepare("SELECT id, date_created, is_read, is_important FROM Messages "
                       "WHERE account_id = :account_id AND custom_id = :custom_id;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, is_read, is_important, url, author, date_created, contents, enclosures, custom_id, account_id) "
                       "VALUES (:feed, :title, :is_read, :is_important, :url, :author, :date_created, :contents, :enclosures, :custom_id, :account_id);");

  if (!database.transaction()) {
    database.rollback();
    qDebug("Transaction start for message downloader failed.");
    return updated_messages;
  }

  foreach (Message message, messages) {
    query_select.bindValue(QSL(":account_id"), account_id);
    query_select.bindValue(QSL(":custom_id"), message.m_customId);

    query_select.exec();

    int id_existing_message = -1;
    qint64 date_existing_message;
    bool is_read_existing_message;
    bool is_important_existing_message;

    if (query_select.next()) {
      id_existing_message = query_select.value(0).toInt();
      date_existing_message = query_select.value(1).value<qint64>();
      is_read_existing_message = query_select.value(2).toBool();
      is_important_existing_message = query_select.value(3).toBool();
    }

    query_select.finish();

    // Now, check if this message is already in the DB.
    if (id_existing_message >= 0) {
      // Message is already in the DB.

      if (message.m_created.toMSecsSinceEpoch() != date_existing_message ||
          message.m_isRead != is_read_existing_message ||
          message.m_isImportant != is_important_existing_message) {
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

        if (query_update.exec()) {
          updated_messages++;
        }

        query_update.finish();

        qDebug("Updating message '%s' in DB.", qPrintable(message.m_title));
      }
    }
    else {
      // Message with this URL is not fetched in this feed yet.
      query_insert.bindValue(QSL(":feed"), feed_id);
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

    updateCounts(true);
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
  }

  return updated_messages;
}
