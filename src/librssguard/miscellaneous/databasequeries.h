// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEQUERIES_H
#define DATABASEQUERIES_H

#include "services/abstract/rootitem.h"

#include "core/messagefilter.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/standardfeed.h"

#include <QMultiMap>
#include <QSqlError>
#include <QSqlQuery>

class DatabaseQueries {
  public:

    // Label operators.
    static bool isLabelAssignedToMessage(const QSqlDatabase& db, Label* label, const Message& msg);
    static bool deassignLabelFromMessage(const QSqlDatabase& db, Label* label, const Message& msg);
    static bool assignLabelToMessage(const QSqlDatabase& db, Label* label, const Message& msg);
    static bool setLabelsForMessage(const QSqlDatabase& db, const QList<Label*>& labels, const Message& msg);
    static QList<Label*> getLabels(const QSqlDatabase& db, int account_id);
    static QList<Label*> getLabelsForMessage(const QSqlDatabase& db, const Message& msg, const QList<Label*> installed_labels);
    static bool updateLabel(const QSqlDatabase& db, Label* label);
    static bool deleteLabel(const QSqlDatabase& db, Label* label);
    static bool createLabel(const QSqlDatabase& db, Label* label, int account_id);

    // Message operators.
    static bool markLabelledMessagesReadUnread(const QSqlDatabase& db, Label* label, RootItem::ReadStatus read);
    static bool markImportantMessagesReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
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
    static bool purgeMessage(const QSqlDatabase& db, int message_id);
    static bool purgeImportantMessages(const QSqlDatabase& db);
    static bool purgeReadMessages(const QSqlDatabase& db);
    static bool purgeOldMessages(const QSqlDatabase& db, int older_than_days);
    static bool purgeRecycleBin(const QSqlDatabase& db);
    static bool purgeMessagesFromBin(const QSqlDatabase& db, bool clear_only_read, int account_id);
    static bool purgeLeftoverMessages(const QSqlDatabase& db, int account_id);

    // Purges message/label assignments where source message or label does not exist.
    // If account ID smaller than 0 is passed, then do this for all accounts.
    static bool purgeLeftoverLabelAssignments(const QSqlDatabase& db, int account_id = -1);
    static bool purgeLabelsAndLabelAssignments(const QSqlDatabase& db, int account_id);

    // Counts of unread/all messages.
    static QMap<QString, QPair<int, int>> getMessageCountsForCategory(const QSqlDatabase& db, const QString& custom_id,
                                                                      int account_id, bool only_total_counts,
                                                                      bool* ok = nullptr);
    static QMap<QString, QPair<int, int>> getMessageCountsForAccount(const QSqlDatabase& db, int account_id,
                                                                     bool only_total_counts, bool* ok = nullptr);
    static int getMessageCountsForFeed(const QSqlDatabase& db, const QString& feed_custom_id, int account_id,
                                       bool only_total_counts, bool* ok = nullptr);
    static int getMessageCountsForLabel(const QSqlDatabase& db, Label* label, int account_id,
                                        bool only_total_counts, bool* ok = nullptr);
    static int getImportantMessageCounts(const QSqlDatabase& db, int account_id,
                                         bool only_total_counts, bool* ok = nullptr);
    static int getMessageCountsForBin(const QSqlDatabase& db, int account_id, bool including_total_counts, bool* ok = nullptr);

    // Get messages (for newspaper view for example).
    static QList<Message> getUndeletedMessagesWithLabel(const QSqlDatabase& db, const Label* label, bool* ok = nullptr);
    static QList<Message> getUndeletedLabelledMessages(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedImportantMessages(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForFeed(const QSqlDatabase& db, const QString& feed_custom_id,
                                                      int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForBin(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QList<Message> getUndeletedMessagesForAccount(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    // Custom ID accumulators.
    static QStringList customIdsOfMessagesFromLabel(const QSqlDatabase& db, Label* label, bool* ok = nullptr);
    static QStringList customIdsOfImportantMessages(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromAccount(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromBin(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static QStringList customIdsOfMessagesFromFeed(const QSqlDatabase& db, const QString& feed_custom_id, int account_id,
                                                   bool* ok = nullptr);

    // Common account methods.
    static int createAccount(const QSqlDatabase& db, const QString& code, bool* ok = nullptr);
    static int updateMessages(QSqlDatabase db, const QList<Message>& messages, const QString& feed_custom_id,
                              int account_id, const QString& url, bool force_update, bool* any_message_changed, bool* ok = nullptr);
    static bool deleteAccount(const QSqlDatabase& db, int account_id);
    static bool deleteAccountData(const QSqlDatabase& db, int account_id, bool delete_messages_too);
    static bool cleanLabelledMessages(const QSqlDatabase& db, bool clean_read_only, Label* label);
    static bool cleanImportantMessages(const QSqlDatabase& db, bool clean_read_only, int account_id);
    static bool cleanFeeds(const QSqlDatabase& db, const QStringList& ids, bool clean_read_only, int account_id);
    static bool storeAccountTree(const QSqlDatabase& db, RootItem* tree_root, int account_id);
    static bool editBaseFeed(const QSqlDatabase& db, int feed_id,
                             Feed::AutoUpdateType auto_update_type, int auto_update_interval,
                             bool is_protected, const QString& username,
                             const QString& password);

    template<typename T>
    static Assignment getCategories(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    template<typename T>
    static Assignment getFeeds(const QSqlDatabase& db, const QList<MessageFilter*>& global_filters,
                               int account_id, bool* ok = nullptr);

    // Message filters operators.
    static bool purgeLeftoverMessageFilterAssignments(const QSqlDatabase& db, int account_id);
    static MessageFilter* addMessageFilter(const QSqlDatabase& db, const QString& title, const QString& script);
    static void removeMessageFilter(const QSqlDatabase& db, int filter_id, bool* ok = nullptr);
    static void removeMessageFilterAssignments(const QSqlDatabase& db, int filter_id, bool* ok = nullptr);
    static QList<MessageFilter*> getMessageFilters(const QSqlDatabase& db, bool* ok = nullptr);
    static QMultiMap<QString, int> messageFiltersInFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static void assignMessageFilterToFeed(const QSqlDatabase& db, const QString& feed_custom_id, int filter_id,
                                          int account_id, bool* ok = nullptr);
    static void updateMessageFilter(const QSqlDatabase& db, MessageFilter* filter, bool* ok = nullptr);
    static void removeMessageFilterFromFeed(const QSqlDatabase& db, const QString& feed_custom_id, int filter_id,
                                            int account_id, bool* ok = nullptr);

    // Standard account.
    static bool deleteFeed(const QSqlDatabase& db, int feed_custom_id, int account_id);
    static bool deleteStandardCategory(const QSqlDatabase& db, int id);
    static int addStandardCategory(const QSqlDatabase& db, int parent_id, int account_id, const QString& title,
                                   const QString& description, const QDateTime& creation_date, const QIcon& icon, bool* ok = nullptr);
    static bool editStandardCategory(const QSqlDatabase& db, int parent_id, int category_id,
                                     const QString& title, const QString& description, const QIcon& icon);
    static int addStandardFeed(const QSqlDatabase& db, int parent_id, int account_id, const QString& title,
                               const QString& description, const QDateTime& creation_date, const QIcon& icon,
                               const QString& encoding, const QString& url, bool is_protected,
                               const QString& username, const QString& password,
                               Feed::AutoUpdateType auto_update_type,
                               int auto_update_interval, StandardFeed::Type feed_format, bool* ok = nullptr);
    static bool editStandardFeed(const QSqlDatabase& db, int parent_id, int feed_id, const QString& title,
                                 const QString& description, const QIcon& icon,
                                 const QString& encoding, const QString& url, bool is_protected,
                                 const QString& username, const QString& password, Feed::AutoUpdateType auto_update_type,
                                 int auto_update_interval, StandardFeed::Type feed_format);
    static QList<ServiceRoot*> getStandardAccounts(const QSqlDatabase& db, bool* ok = nullptr);

    template<typename T>
    static void fillFeedData(T* feed, const QSqlRecord& sql_record);

    // Nextcloud account.
    static QList<ServiceRoot*> getOwnCloudAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool deleteOwnCloudAccount(const QSqlDatabase& db, int account_id);
    static bool overwriteOwnCloudAccount(const QSqlDatabase& db, const QString& username, const QString& password,
                                         const QString& url, bool force_server_side_feed_update, int batch_size,
                                         bool download_only_unread_messages, int account_id);
    static bool createOwnCloudAccount(const QSqlDatabase& db, int id_to_assign, const QString& username, const QString& password,
                                      const QString& url, bool force_server_side_feed_update,
                                      bool download_only_unread_messages, int batch_size);

    // TT-RSS acccount.
    static QList<ServiceRoot*> getTtRssAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool deleteTtRssAccount(const QSqlDatabase& db, int account_id);
    static bool overwriteTtRssAccount(const QSqlDatabase& db, const QString& username, const QString& password,
                                      bool auth_protected, const QString& auth_username, const QString& auth_password,
                                      const QString& url, bool force_server_side_feed_update,
                                      bool download_only_unread_messages, int account_id);
    static bool createTtRssAccount(const QSqlDatabase& db, int id_to_assign, const QString& username,
                                   const QString& password, bool auth_protected, const QString& auth_username,
                                   const QString& auth_password, const QString& url,
                                   bool force_server_side_feed_update, bool download_only_unread_messages);

    // Gmail account.
    static QStringList getAllRecipients(const QSqlDatabase& db, int account_id);
    static bool deleteGmailAccount(const QSqlDatabase& db, int account_id);
    static bool storeNewGmailTokens(const QSqlDatabase& db, const QString& refresh_token, int account_id);
    static QList<ServiceRoot*> getGmailAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool overwriteGmailAccount(const QSqlDatabase& db, const QString& username, const QString& app_id,
                                      const QString& app_key, const QString& redirect_url, const QString& refresh_token,
                                      int batch_size, int account_id);
    static bool createGmailAccount(const QSqlDatabase& db, int id_to_assign, const QString& username,
                                   const QString& app_id, const QString& app_key, const QString& redirect_url,
                                   const QString& refresh_token, int batch_size);

    // Inoreader account.
    static bool deleteInoreaderAccount(const QSqlDatabase& db, int account_id);
    static bool storeNewInoreaderTokens(const QSqlDatabase& db, const QString& refresh_token, int account_id);
    static QList<ServiceRoot*> getInoreaderAccounts(const QSqlDatabase& db, bool* ok = nullptr);
    static bool overwriteInoreaderAccount(const QSqlDatabase& db, const QString& username, const QString& app_id,
                                          const QString& app_key, const QString& redirect_url, const QString& refresh_token,
                                          int batch_size, int account_id);
    static bool createInoreaderAccount(const QSqlDatabase& db, int id_to_assign, const QString& username,
                                       const QString& app_id, const QString& app_key, const QString& redirect_url,
                                       const QString& refresh_token, int batch_size);

  private:
    static QString unnulifyString(const QString& str);

    explicit DatabaseQueries() = default;
};

template<typename T>
void DatabaseQueries::fillFeedData(T* feed, const QSqlRecord& sql_record) {
  Q_UNUSED(feed)
  Q_UNUSED(sql_record)
}

template<>
inline void DatabaseQueries::fillFeedData(StandardFeed* feed, const QSqlRecord& sql_record) {
  Q_UNUSED(feed)
  Q_UNUSED(sql_record)
}

template<typename T>
Assignment DatabaseQueries::getCategories(const QSqlDatabase& db, int account_id, bool* ok) {
  Assignment categories;

  // Obtain data for categories from the database.
  QSqlQuery query_categories(db);

  query_categories.setForwardOnly(true);
  query_categories.prepare(QSL("SELECT * FROM Categories WHERE account_id = :account_id;"));
  query_categories.bindValue(QSL(":account_id"), account_id);

  if (!query_categories.exec()) {
    qFatal("Query for obtaining categories failed. Error message: '%s'.", qPrintable(query_categories.lastError().text()));

    if (ok != nullptr) {
      *ok = false;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = true;
    }
  }

  while (query_categories.next()) {
    AssignmentItem pair;

    pair.first = query_categories.value(CAT_DB_PARENT_ID_INDEX).toInt();
    pair.second = new T(query_categories.record());
    categories << pair;
  }

  return categories;
}

template<typename T>
Assignment DatabaseQueries::getFeeds(const QSqlDatabase& db, const QList<MessageFilter*>& global_filters,
                                     int account_id, bool* ok) {
  Assignment feeds;

  // All categories are now loaded.
  QSqlQuery query(db);
  auto filters_in_feeds = messageFiltersInFeeds(db, account_id);

  query.setForwardOnly(true);
  query.prepare(QSL("SELECT * FROM Feeds WHERE account_id = :account_id;"));
  query.bindValue(QSL(":account_id"), account_id);

  if (!query.exec()) {
    qFatal("Query for obtaining feeds failed. Error message: '%s'.", qPrintable(query.lastError().text()));

    if (ok != nullptr) {
      *ok = false;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = true;
    }
  }

  while (query.next()) {
    AssignmentItem pair;

    pair.first = query.value(FDS_DB_CATEGORY_INDEX).toInt();

    Feed* feed = new T(query.record());

    if (filters_in_feeds.contains(feed->customId())) {
      auto all_filters_for_this_feed = filters_in_feeds.values(feed->customId());

      for (MessageFilter* fltr : global_filters) {
        if (all_filters_for_this_feed.contains(fltr->id())) {
          feed->appendMessageFilter(fltr);
        }
      }
    }

    fillFeedData<T>(static_cast<T*>(feed), query.record());

    pair.second = feed;

    feeds << pair;
  }

  return feeds;
}

#endif // DATABASEQUERIES_H
