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

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "gui/dialogs/formmain.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/ttrsscategory.h"
#include "services/tt-rss/gui/formeditfeed.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QPointer>


TtRssFeed::TtRssFeed(RootItem *parent)
  : Feed(parent), m_customId(NO_PARENT_CATEGORY) {
}

TtRssFeed::TtRssFeed(const QSqlRecord &record) : Feed(NULL) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
}

TtRssFeed::~TtRssFeed() {
}

QString TtRssFeed::hashCode() const {
  return
      QString::number(kind()) + QL1S("-") +
      QString::number(getParentServiceRoot()->accountId()) + QL1S("-") +
      QString::number(customId());
}

TtRssServiceRoot *TtRssFeed::serviceRoot() const {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

QVariant TtRssFeed::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        QString auto_update_string;

        switch (autoUpdateType()) {
          case DontAutoUpdate:
            //: Describes feed auto-update status.
            auto_update_string = tr("does not use auto-update");
            break;

          case DefaultAutoUpdate:
            //: Describes feed auto-update status.
            auto_update_string = tr("uses global settings");
            break;

          case SpecificAutoUpdate:
          default:
            //: Describes feed auto-update status.
            auto_update_string = tr("uses specific settings "
                                    "(%n minute(s) to next auto-update)",
                                    0,
                                    autoUpdateRemainingInterval());
            break;
        }

        //: Tooltip for feed.
        return tr("%1"
                  "%2\n\n"
                  "Auto-update status: %3").arg(title(),
                                                description().isEmpty() ? QString() : QString('\n') + description(),
                                                auto_update_string);
      }
      else {
        return Feed::data(column, role);
      }

    default:
      return Feed::data(column, role);
  }
}

void TtRssFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);

  query_all.setForwardOnly(true);

  if (including_total_count) {
    query_all.prepare("SELECT count(*) FROM Messages "
                      "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
    query_all.bindValue(QSL(":feed"), customId());
    query_all.bindValue(QSL(":account_id"), serviceRoot()->accountId());

    if (query_all.exec() && query_all.next()) {
      setCountOfAllMessages(query_all.value(0).toInt());
    }
  }

  query_all.prepare("SELECT count(*) FROM Messages "
                    "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 0 AND account_id = :account_id;");
  query_all.bindValue(QSL(":feed"), customId());
  query_all.bindValue(QSL(":account_id"), serviceRoot()->accountId());

  // Obtain count of unread messages.
  if (query_all.exec() && query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (status() == NewMessages && new_unread_count < countOfUnreadMessages()) {
      setStatus(Normal);
    }

    setCountOfUnreadMessages(new_unread_count);
  }
}

bool TtRssFeed::canBeEdited() const {
  return true;
}

bool TtRssFeed::editViaGui() {
  QPointer<FormEditFeed> form_pointer = new FormEditFeed(serviceRoot(), qApp->mainForm());

  form_pointer.data()->execForEdit(this);
  delete form_pointer.data();
  return false;
}

bool TtRssFeed::canBeDeleted() const {
  return true;
}

bool TtRssFeed::deleteViaGui() {
  if (removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
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
  int account_id = serviceRoot()->accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);

  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT * "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;");

  query_read_msg.bindValue(QSL(":feed"), customId());
  query_read_msg.bindValue(QSL(":account_id"), account_id);

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

bool TtRssFeed::editItself(TtRssFeed *new_feed_data) {
  QSqlDatabase database = qApp->database()->connection("aa", DatabaseFactory::FromSettings);
  QSqlQuery query_update(database);

  query_update.setForwardOnly(true);
  query_update.prepare("UPDATE Feeds "
                       "SET update_type = :update_type, update_interval = :update_interval "
                       "WHERE id = :id;");

  query_update.bindValue(QSL(":update_type"), (int) new_feed_data->autoUpdateType());
  query_update.bindValue(QSL(":update_interval"), new_feed_data->autoUpdateInitialInterval());
  query_update.bindValue(QSL(":id"), id());

  if (query_update.exec()) {
    setAutoUpdateType(new_feed_data->autoUpdateType());
    setAutoUpdateInitialInterval(new_feed_data->autoUpdateInitialInterval());

    return true;
  }
  else {
    return false;
  }
}

bool TtRssFeed::removeItself() {
  TtRssUnsubscribeFeedResponse response = serviceRoot()->network()->unsubscribeFeed(customId());

  if (response.code() == UFF_OK) {
    // Feed was removed online from server, remove local data.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query_remove(database);

    query_remove.setForwardOnly(true);

    // Remove all messages from this standard feed.
    query_remove.prepare(QSL("DELETE FROM Messages WHERE feed = :feed;"));
    query_remove.bindValue(QSL(":feed"), customId());

    if (!query_remove.exec()) {
      return false;
    }

    // Remove feed itself.
    query_remove.prepare(QSL("DELETE FROM Feeds WHERE custom_id = :feed;"));
    query_remove.bindValue(QSL(":feed"), customId());

    return query_remove.exec();
  }
  else {
    qWarning("TT-RSS: Unsubscribing from feed failed, received JSON: '%s'", qPrintable(response.toString()));

    return false;
  }
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
