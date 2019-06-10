// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEQUERIES_H
#define DATABASEQUERIES_H

#include "services/abstract/rootitem.h"

#include "services/abstract/serviceroot.h"
#include "services/standard/standardfeed.h"

#include <QSqlQuery>

class DatabaseQueries {
  public:

    // Mark read/unread/starred/delete messages.
    static bool markMessagesReadUnread(const QSqlDatabase& db, const QStringList& ids, RootItem::ReadStatus read);
    static bool markMessageImportant(const QSqlDatabase& db, int id, RootItem::Importance importance);
    static bool markFeedsReadUnread(const QSqlDatabase& db, const QStringList& ids, int account_id, RootItem::ReadStatus read);
    static bool markBinReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
    static bool markAccountReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
    static bool switchMessagesImportance(const QSqlDatabase& db, const QStringList& ids);
    static bool permanentlyDeleteMessages(const QSqlDatabase& db, const QStringList& ids);
    static bool deleteOrRestoreMessagesToFromBin(const QSqlDatabase& db, const QStringList& ids, bool deleted);
    static bool restoreBin(const QSqlDatabase& db, int account_id);

    // Purge database.
    static bool purgeImportantMessages(const QSqlDatabase& db);
    static bool purgeReadMessages(const QSqlDatabase& db);
    static bool purgeOldMessages(const QSqlDatabase& db, int older_than_days);
    static bool purgeRecycleBin(const QSqlDatabase& db);
    static bool purgeMessagesFromBin(const QSqlDatabase& db, bool clear_only_read, int account_id);
    static bool purgeLeftoverMessages(const QSqlDatabase& db, int account_id);

    // Obtain counts of unread/all messages.
    static QMap<QString, QPair<int, int>> getMessageCountsForCategory(const QSqlDatabase& db, const QString& custom_id, int account_id,
                                                                      bool including_total_counts, bool* ok = nullptr);
    static QMap<QString, QPair<int, int>> getMessageCountsForAccount(const QSqlDatabase& db, int account_id,
                                                                     bool including_total_counts, bool* ok = nullptr);
    static int getMessageCountsForFeed(const QSqlDatabase& db, const QString& feed_custom_id, int account_id,
                                       bool including_total_counts, bool* ok = nullptr);
    static int getMessageCountsForBin(const QSqlDatabase& db, int account_id, bool including_total_counts, bool* ok = nullptr);

    // Get messages (for newspaper view for example).
    static QList<Message> getUndeletedMessagesForFeed(const QSqlDatabase& db, const QString& feed_custom_id, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForBin(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForAccount(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    // Custom ID accumulators.
    static QStringList customIdsOfMessagesFromAccount(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromBin(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromFeed(const QSqlDatabase& db, const QString& feed_custom_id, int account_id, bool* ok = nullptr);

    // Common accounts methods.
    static int updateMessages(QSqlDatabase db, const QList<Message>& messages, const QString& feed_custom_id,
                              int account_id, const QString& url, bool* any_message_changed, bool* ok = nullptr);
    static bool deleteAccount(const QSqlDatabase& db, int account_id);
    static bool deleteAccountData(const QSqlDatabase& db, int account_id, bool delete_messages_too);
    static bool cleanFeeds(const QSqlDatabase& db, const QStringList& ids, bool clean_read_only, int account_id);
    static bool storeAccountTree(const QSqlDatabase& db, RootItem* tree_root, int account_id);
    static bool editBaseFeed(const QSqlDatabase& db, int feed_id, Feed::AutoUpdateType auto_update_type,
                             int auto_update_interval);
    static Assignment getCategories(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    // Gmail account.
    static Assignment getGmailFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static bool deleteGmailAccount(const QSqlDatabase& db, int account_id);
    static QList<ServiceRoot*> getGmailAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool overwriteGmailAccount(const QSqlDatabase& db, const QString& username, const QString& app_id,
                                      const QString& app_key, const QString& redirect_url, const QString& refresh_token,
                                      int batch_size, int account_id);
    static bool createGmailAccount(const QSqlDatabase& db, int id_to_assign, const QString& username,
                                   const QString& app_id, const QString& app_key, const QString& redirect_url,
                                   const QString& refresh_token, int batch_size);

    // Inoreader account.
    static bool deleteInoreaderAccount(const QSqlDatabase& db, int account_id);
    static Assignment getInoreaderFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static bool storeNewInoreaderTokens(const QSqlDatabase& db, const QString& refresh_token, int account_id);
    static QList<ServiceRoot*> getInoreaderAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool overwriteInoreaderAccount(const QSqlDatabase& db, const QString& username, const QString& app_id,
                                          const QString& app_key, const QString& redirect_url, const QString& refresh_token,
                                          int batch_size, int account_id);
    static bool createInoreaderAccount(const QSqlDatabase& db, int id_to_assign, const QString& username,
                                       const QString& app_id, const QString& app_key, const QString& redirect_url,
                                       const QString& refresh_token, int batch_size);

    // ownCloud account.
    static QList<ServiceRoot*> getOwnCloudAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool deleteOwnCloudAccount(const QSqlDatabase& db, int account_id);
    static bool overwriteOwnCloudAccount(const QSqlDatabase& db, const QString& username, const QString& password,
                                         const QString& url, bool force_server_side_feed_update, int batch_size, int account_id);
    static bool createOwnCloudAccount(const QSqlDatabase& db, int id_to_assign, const QString& username, const QString& password,
                                      const QString& url, bool force_server_side_feed_update, int batch_size);
    static int createAccount(const QSqlDatabase& db, const QString& code, bool* ok = nullptr);
    static Assignment getOwnCloudFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    // Standard account.
    static bool deleteFeed(const QSqlDatabase& db, int feed_custom_id, int account_id);
    static bool deleteCategory(const QSqlDatabase& db, int id);
    static int addCategory(const QSqlDatabase& db, int parent_id, int account_id, const QString& title,
                           const QString& description, const QDateTime& creation_date, const QIcon& icon, bool* ok = nullptr);
    static bool editCategory(const QSqlDatabase& db, int parent_id, int category_id,
                             const QString& title, const QString& description, const QIcon& icon);
    static int addFeed(const QSqlDatabase& db, int parent_id, int account_id, const QString& title,
                       const QString& description, const QDateTime& creation_date, const QIcon& icon,
                       const QString& encoding, const QString& url, bool is_protected,
                       const QString& username, const QString& password,
                       Feed::AutoUpdateType auto_update_type,
                       int auto_update_interval, StandardFeed::Type feed_format, bool* ok = nullptr);
    static bool editFeed(const QSqlDatabase& db, int parent_id, int feed_id, const QString& title,
                         const QString& description, const QIcon& icon,
                         const QString& encoding, const QString& url, bool is_protected,
                         const QString& username, const QString& password, Feed::AutoUpdateType auto_update_type,
                         int auto_update_interval, StandardFeed::Type feed_format);
    static QList<ServiceRoot*> getAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static Assignment getStandardCategories(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static Assignment getStandardFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    // TT-RSS acccount.
    static QList<ServiceRoot*> getTtRssAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool deleteTtRssAccount(const QSqlDatabase& db, int account_id);
    static bool overwriteTtRssAccount(const QSqlDatabase& db, const QString& username, const QString& password,
                                      bool auth_protected, const QString& auth_username, const QString& auth_password,
                                      const QString& url, bool force_server_side_feed_update, int account_id);
    static bool createTtRssAccount(const QSqlDatabase& db, int id_to_assign, const QString& username,
                                   const QString& password, bool auth_protected, const QString& auth_username,
                                   const QString& auth_password, const QString& url,
                                   bool force_server_side_feed_update);
    static Assignment getTtRssFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

  private:
    static QString unnulifyString(const QString& str);

    explicit DatabaseQueries();
};

#endif // DATABASEQUERIES_H
