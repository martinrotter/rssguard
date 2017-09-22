// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef DATABASEQUERIES_H
#define DATABASEQUERIES_H

#include "services/abstract/rootitem.h"

#include "services/abstract/serviceroot.h"
#include "services/standard/standardfeed.h"

#include <QSqlQuery>

class DatabaseQueries {
  public:

    // Mark read/unread/starred/delete messages.
    static bool markMessagesReadUnread(QSqlDatabase db, const QStringList& ids, RootItem::ReadStatus read);
    static bool markMessageImportant(QSqlDatabase db, int id, RootItem::Importance importance);
    static bool markFeedsReadUnread(QSqlDatabase db, const QStringList& ids, int account_id, RootItem::ReadStatus read);
    static bool markBinReadUnread(QSqlDatabase db, int account_id, RootItem::ReadStatus read);
    static bool markAccountReadUnread(QSqlDatabase db, int account_id, RootItem::ReadStatus read);
    static bool switchMessagesImportance(QSqlDatabase db, const QStringList& ids);
    static bool permanentlyDeleteMessages(QSqlDatabase db, const QStringList& ids);
    static bool deleteOrRestoreMessagesToFromBin(QSqlDatabase db, const QStringList& ids, bool deleted);
    static bool restoreBin(QSqlDatabase db, int account_id);

    // Purge database.
    static bool purgeImportantMessages(QSqlDatabase db);
    static bool purgeReadMessages(QSqlDatabase db);
    static bool purgeOldMessages(QSqlDatabase db, int older_than_days);
    static bool purgeRecycleBin(QSqlDatabase db);
    static bool purgeMessagesFromBin(QSqlDatabase db, bool clear_only_read, int account_id);
    static bool purgeLeftoverMessages(QSqlDatabase db, int account_id);

    // Obtain counts of unread/all messages.
    static QMap<QString, QPair<int, int>> getMessageCountsForCategory(QSqlDatabase db, const QString& custom_id, int account_id,
                                                                      bool including_total_counts, bool* ok = nullptr);
    static QMap<QString, QPair<int, int>> getMessageCountsForAccount(QSqlDatabase db, int account_id,
                                                                     bool including_total_counts, bool* ok = nullptr);
    static int getMessageCountsForFeed(QSqlDatabase db, const QString& feed_custom_id, int account_id,
                                       bool including_total_counts, bool* ok = nullptr);
    static int getMessageCountsForBin(QSqlDatabase db, int account_id, bool including_total_counts, bool* ok = nullptr);

    // Get messages (for newspaper view for example).
    static QList<Message> getUndeletedMessagesForFeed(QSqlDatabase db, const QString& feed_custom_id, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForBin(QSqlDatabase db, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForAccount(QSqlDatabase db, int account_id, bool* ok = nullptr);

    // Custom ID accumulators.
    static QStringList customIdsOfMessagesFromAccount(QSqlDatabase db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromBin(QSqlDatabase db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromFeed(QSqlDatabase db, const QString& feed_custom_id, int account_id, bool* ok = nullptr);

    // Common accounts methods.
    static int updateMessages(QSqlDatabase db, const QList<Message>& messages, const QString& feed_custom_id,
                              int account_id, const QString& url, bool* any_message_changed, bool* ok = nullptr);
    static bool deleteAccount(QSqlDatabase db, int account_id);
    static bool deleteAccountData(QSqlDatabase db, int account_id, bool delete_messages_too);
    static bool cleanFeeds(QSqlDatabase db, const QStringList& ids, bool clean_read_only, int account_id);
    static bool storeAccountTree(QSqlDatabase db, RootItem* tree_root, int account_id);
    static bool editBaseFeed(QSqlDatabase db, int feed_id, Feed::AutoUpdateType auto_update_type,
                             int auto_update_interval);
    static Assignment getCategories(QSqlDatabase db, int account_id, bool* ok = nullptr);

    // Inoreader account.
    static QList<ServiceRoot*> getInoreaderAccounts(QSqlDatabase db, bool* ok = nullptr);
    static bool overwriteInoreaderAccount(QSqlDatabase db, const QString& username, const QString& access_token,
                                          const QString& refresh_token, int batch_size, int account_id);
    static bool createInoreaderAccount(QSqlDatabase db, int id_to_assign, const QString& username,
                                       const QString& access_token, const QString& refresh_token, int batch_size);

    // ownCloud account.
    static QList<ServiceRoot*> getOwnCloudAccounts(QSqlDatabase db, bool* ok = nullptr);
    static bool deleteOwnCloudAccount(QSqlDatabase db, int account_id);
    static bool overwriteOwnCloudAccount(QSqlDatabase db, const QString& username, const QString& password,
                                         const QString& url, bool force_server_side_feed_update, int batch_size, int account_id);
    static bool createOwnCloudAccount(QSqlDatabase db, int id_to_assign, const QString& username, const QString& password,
                                      const QString& url, bool force_server_side_feed_update, int batch_size);
    static int createAccount(QSqlDatabase db, const QString& code, bool* ok = nullptr);
    static Assignment getOwnCloudFeeds(QSqlDatabase db, int account_id, bool* ok = nullptr);

    // Standard account.
    static bool deleteFeed(QSqlDatabase db, int feed_custom_id, int account_id);
    static bool deleteCategory(QSqlDatabase db, int id);
    static int addCategory(QSqlDatabase db, int parent_id, int account_id, const QString& title,
                           const QString& description, QDateTime creation_date, const QIcon& icon, bool* ok = nullptr);
    static bool editCategory(QSqlDatabase db, int parent_id, int category_id,
                             const QString& title, const QString& description, const QIcon& icon);
    static int addFeed(QSqlDatabase db, int parent_id, int account_id, const QString& title,
                       const QString& description, QDateTime creation_date, const QIcon& icon,
                       const QString& encoding, const QString& url, bool is_protected,
                       const QString& username, const QString& password,
                       Feed::AutoUpdateType auto_update_type,
                       int auto_update_interval, StandardFeed::Type feed_format, bool* ok = nullptr);
    static bool editFeed(QSqlDatabase db, int parent_id, int feed_id, const QString& title,
                         const QString& description, const QIcon& icon,
                         const QString& encoding, const QString& url, bool is_protected,
                         const QString& username, const QString& password, Feed::AutoUpdateType auto_update_type,
                         int auto_update_interval, StandardFeed::Type feed_format);
    static QList<ServiceRoot*> getAccounts(QSqlDatabase db, bool* ok = nullptr);
    static Assignment getStandardCategories(QSqlDatabase db, int account_id, bool* ok = nullptr);
    static Assignment getStandardFeeds(QSqlDatabase db, int account_id, bool* ok = nullptr);

    // TT-RSS acccount.
    static QList<ServiceRoot*> getTtRssAccounts(QSqlDatabase db, bool* ok = nullptr);
    static bool deleteTtRssAccount(QSqlDatabase db, int account_id);
    static bool overwriteTtRssAccount(QSqlDatabase db, const QString& username, const QString& password,
                                      bool auth_protected, const QString& auth_username, const QString& auth_password,
                                      const QString& url, bool force_server_side_feed_update, int account_id);
    static bool createTtRssAccount(QSqlDatabase db, int id_to_assign, const QString& username,
                                   const QString& password, bool auth_protected, const QString& auth_username,
                                   const QString& auth_password, const QString& url,
                                   bool force_server_side_feed_update);
    static Assignment getTtRssFeeds(QSqlDatabase db, int account_id, bool* ok = nullptr);

  private:
    explicit DatabaseQueries();
};

#endif // DATABASEQUERIES_H
