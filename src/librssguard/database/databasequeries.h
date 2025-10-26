// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEQUERIES_H
#define DATABASEQUERIES_H

#include "definitions/typedefs.h"
#include "exceptions/sqlexception.h"
#include "filtering/messagefilter.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/search.h"
#include "services/abstract/serviceroot.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMultiMap>
#include <QSqlError>
#include <QSqlQuery>

class RSSGUARD_DLLSPEC DatabaseQueries {
  public:
    static QStringList messageTableAttributes();

    // Custom data serializers.
    static QString serializeCustomData(const QVariantHash& data);
    static QVariantHash deserializeCustomData(const QString& data);

    // Labels
    static void deassignLabelFromMessage(const QSqlDatabase& db, Label* label, const Message& msg);
    static void assignLabelToMessage(const QSqlDatabase& db, Label* label, const Message& msg);
    static void setLabelsForMessage(const QSqlDatabase& db, const QList<Label*>& labels, const Message& msg);
    static QList<Label*> getLabelsForAccount(const QSqlDatabase& db, int account_id);
    static void updateLabel(const QSqlDatabase& db, Label* label);
    static void deleteLabel(const QSqlDatabase& db, Label* label);
    static void createLabel(const QSqlDatabase& db, Label* label, int account_id, int new_label_id = 0);
    static void purgeLabelAssignments(const QSqlDatabase& db, Label* label);

    // Probes
    static void createProbe(const QSqlDatabase& db, Search* probe, int account_id);
    static QList<Search*> getProbesForAccount(const QSqlDatabase& db, int account_id);
    static void deleteProbe(const QSqlDatabase& db, Search* probe);
    static void updateProbe(const QSqlDatabase& db, Search* probe);

    // Read & unread & important articles.
    static void markProbeReadUnread(const QSqlDatabase& db, Search* probe, RootItem::ReadStatus read);
    static void markAllLabelledMessagesReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
    static void markLabelledMessagesReadUnread(const QSqlDatabase& db, Label* label, RootItem::ReadStatus read);
    static void markImportantMessagesReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
    static void markUnreadMessagesRead(const QSqlDatabase& db, int account_id);
    static void markMessagesReadUnread(const QSqlDatabase& db, const QStringList& ids, RootItem::ReadStatus read);
    static void markMessageImportant(const QSqlDatabase& db, int id, RootItem::Importance importance);
    static void markFeedsReadUnread(const QSqlDatabase& db,
                                    const QStringList& ids,
                                    int account_id,
                                    RootItem::ReadStatus read);
    static void markBinReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
    static void markAccountReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read);
    static void switchMessagesImportance(const QSqlDatabase& db, const QStringList& ids);

    // Delete & restore articles.
    static void permanentlyDeleteMessages(const QSqlDatabase& db, const QStringList& ids);
    static void deleteOrRestoreMessagesToFromBin(const QSqlDatabase& db, const QStringList& ids, bool deleted);
    static void restoreBin(const QSqlDatabase& db, int account_id);

    // Purge database.
    static bool removeUnwantedArticlesFromFeed(const QSqlDatabase& db,
                                               const Feed* feed,
                                               const Feed::ArticleIgnoreLimit& feed_setup,
                                               const Feed::ArticleIgnoreLimit& app_setup);
    static void purgeFeedArticles(const QSqlDatabase& database, const QList<Feed*>& feeds);
    static void purgeMessage(const QSqlDatabase& db, int message_id);
    static void purgeImportantMessages(const QSqlDatabase& db);
    static void purgeReadMessages(const QSqlDatabase& db);
    static void purgeOldMessages(const QSqlDatabase& db, int older_than_days);
    static void purgeRecycleBin(const QSqlDatabase& db);
    static void purgeMessagesFromBin(const QSqlDatabase& db, bool clear_only_read, int account_id);
    static void purgeLeftoverMessages(const QSqlDatabase& db, int account_id);

    // Counts of unread/all messages.
    static QMap<int, ArticleCounts> getMessageCountsForCategory(const QSqlDatabase& db,
                                                                const QString& custom_id,
                                                                int account_id,
                                                                bool include_total_counts);
    static QMap<int, ArticleCounts> getMessageCountsForAccount(const QSqlDatabase& db,
                                                               int account_id,
                                                               bool include_total_counts);
    static ArticleCounts getMessageCountsForFeed(const QSqlDatabase& db, int feed_id, int account_id);
    static ArticleCounts getMessageCountsForLabel(const QSqlDatabase& db, Label* label, int account_id);
    static QMap<int, ArticleCounts> getMessageCountsForAllLabels(const QSqlDatabase& db, int account_id);
    static ArticleCounts getImportantMessageCounts(const QSqlDatabase& db, int account_id);
    static int getUnreadMessageCounts(const QSqlDatabase& db, int account_id);
    static ArticleCounts getMessageCountsForBin(const QSqlDatabase& db, int account_id);

    // Get messages (for newspaper view for example).
    static QList<Message> getUndeletedMessagesForAccount(const QSqlDatabase& db,
                                                         const QHash<QString, Label*>& labels,
                                                         int account_id);
    static QList<Message> getUndeletedMessagesForFeed(const QSqlDatabase& db,
                                                      int feed_id,
                                                      const QHash<QString, Label*>& labels,
                                                      int account_id);

    // Custom ID accumulators.
    static int highestPrimaryIdFeeds(const QSqlDatabase& db);
    static int highestPrimaryIdLabels(const QSqlDatabase& db);
    static QStringList bagOfMessages(const QSqlDatabase& db, ServiceRoot::BagOfMessages bag, const Feed* feed);
    static QHash<QString, QStringList> bagsOfMessages(const QSqlDatabase& db, const QList<Label*>& labels);
    static QStringList customIdsOfMessagesFromLabel(const QSqlDatabase& db,
                                                    Label* label,
                                                    RootItem::ReadStatus target_read);
    static QStringList customIdsOfMessagesFromProbe(const QSqlDatabase& db,
                                                    Search* probe,
                                                    RootItem::ReadStatus target_read);
    static QStringList customIdsOfImportantMessages(const QSqlDatabase& db,
                                                    RootItem::ReadStatus target_read,
                                                    int account_id);
    static QStringList customIdsOfUnreadMessages(const QSqlDatabase& db, int account_id);
    static QStringList customIdsOfMessagesFromAccount(const QSqlDatabase& db,
                                                      RootItem::ReadStatus target_read,
                                                      int account_id);
    static QStringList customIdsOfMessagesFromBin(const QSqlDatabase& db,
                                                  RootItem::ReadStatus target_read,
                                                  int account_id);
    static QStringList customIdsOfMessagesFromFeed(const QSqlDatabase& db,
                                                   int feed_id,
                                                   RootItem::ReadStatus target_read,
                                                   int account_id);

    // Common account methods.
    template <typename T>
    static QList<ServiceRoot*> getAccounts(const QSqlDatabase& db, const QString& code);

    template <typename Categ, typename Fee>
    static void loadRootFromDatabase(ServiceRoot* root);
    static void storeNewOauthTokens(const QSqlDatabase& db, const QString& refresh_token, int account_id);
    static void createOverwriteAccount(const QSqlDatabase& db, ServiceRoot* account);

    static UpdatedArticles updateMessages(QSqlDatabase& db,
                                          QList<Message>& messages,
                                          Feed* feed,
                                          bool force_update,
                                          bool force_insert,
                                          QMutex* db_mutex);
    static void deleteAccount(const QSqlDatabase& db, ServiceRoot* account);
    static void deleteAccountData(const QSqlDatabase& db,
                                  int account_id,
                                  bool delete_messages_too,
                                  bool delete_labels_too);

    static void cleanLabelledMessages(const QSqlDatabase& db, bool clean_read_only, Label* label);
    static void cleanProbedMessages(const QSqlDatabase& db, bool clean_read_only, Search* probe);
    static void cleanImportantMessages(const QSqlDatabase& db, bool clean_read_only, int account_id);
    static void cleanUnreadMessages(const QSqlDatabase& db, int account_id);
    static void cleanFeeds(const QSqlDatabase& db, const QStringList& ids, bool clean_read_only, int account_id);

    // TODO: pokračovat?
    static void storeAccountTree(const QSqlDatabase& db,
                                 RootItem* tree_root,
                                 int next_feed_id,
                                 int next_label_id,
                                 int account_id);
    static void createOverwriteFeed(const QSqlDatabase& db,
                                    Feed* feed,
                                    int account_id,
                                    int new_parent_id,
                                    int new_feed_id = 0);
    static void createOverwriteCategory(const QSqlDatabase& db, Category* category, int account_id, int new_parent_id);
    static bool deleteFeed(const QSqlDatabase& db, Feed* feed, int account_id);
    static bool deleteCategory(const QSqlDatabase& db, Category* category);

    template <typename T>
    static Assignment getCategories(const QSqlDatabase& db, int account_id, bool* ok = nullptr);

    template <typename T>
    static Assignment getFeeds(const QSqlDatabase& db,
                               const QList<MessageFilter*>& global_filters,
                               int account_id,
                               bool* ok = nullptr);

    // Item order methods.
    static void moveItem(RootItem* item, bool move_top, bool move_bottom, int move_index, const QSqlDatabase& db);

    // Message filters operators.
    static void moveMessageFilter(QList<MessageFilter*> all_filters,
                                  MessageFilter* filter,
                                  bool move_top,
                                  bool move_bottom,
                                  int move_index,
                                  const QSqlDatabase& db);
    static bool purgeLeftoverMessageFilterAssignments(const QSqlDatabase& db, int account_id);
    static void purgeLeftoverLabelAssignments(const QSqlDatabase& db, int account_id = -1);
    static MessageFilter* addMessageFilter(const QSqlDatabase& db, const QString& title, const QString& script);
    static void removeMessageFilter(const QSqlDatabase& db, int filter_id, bool* ok = nullptr);
    static void removeMessageFilterAssignments(const QSqlDatabase& db, int filter_id, bool* ok = nullptr);
    static QList<MessageFilter*> getMessageFilters(const QSqlDatabase& db, bool* ok = nullptr);
    static void assignMessageFilterToFeed(const QSqlDatabase& db,
                                          int feed_id,
                                          int filter_id,
                                          int account_id,
                                          bool* ok = nullptr);
    static void updateMessageFilter(const QSqlDatabase& db, MessageFilter* filter, bool* ok = nullptr);
    static QMultiMap<int, int> messageFiltersInFeeds(const QSqlDatabase& db, int account_id, bool* ok = nullptr);
    static void removeMessageFilterFromFeed(const QSqlDatabase& db,
                                            int feed_id,
                                            int filter_id,
                                            int account_id,
                                            bool* ok = nullptr);

    // Gmail account.
    static QStringList getAllGmailRecipients(const QSqlDatabase& db, int account_id);

  private:
    static QString unnulifyString(const QString& str);

    explicit DatabaseQueries() = default;
};

template <typename T>
QList<ServiceRoot*> DatabaseQueries::getAccounts(const QSqlDatabase& db, const QString& code) {
  QSqlQuery query(db);
  QList<ServiceRoot*> roots;

  if (query.exec(QSL("SELECT * FROM Accounts WHERE type = '%1';").arg(code))) {
    DatabaseFactory::logLastExecutedQuery(query);

    while (query.next()) {
      ServiceRoot* root = new T();

      // Load common data.
      root->setAccountId(query.value(QSL("id")).toInt());
      root->setSortOrder(query.value(QSL("ordr")).toInt());

      QNetworkProxy proxy(QNetworkProxy::ProxyType(query.value(QSL("proxy_type")).toInt()),
                          query.value(QSL("proxy_host")).toString(),
                          query.value(QSL("proxy_port")).toInt(),
                          query.value(QSL("proxy_username")).toString(),
                          TextFactory::decrypt(query.value(QSL("proxy_password")).toString()));

      root->setNetworkProxy(proxy);
      root->setCustomDatabaseData(deserializeCustomData(query.value(QSL("custom_data")).toString()));

      roots.append(root);
    }
  }
  else {
    qCriticalNN << LOGSEC_DB << "Loading of accounts with code" << QUOTE_W_SPACE(code)
                << "failed with error:" << QUOTE_W_SPACE_DOT(query.lastError().text());

    throw SqlException(query.lastError());
  }

  return roots;
}

template <typename T>
Assignment DatabaseQueries::getCategories(const QSqlDatabase& db, int account_id, bool* ok) {
  Assignment categories;

  // Obtain data for categories from the database.
  QSqlQuery query_categories(db);

  query_categories.setForwardOnly(true);
  query_categories.prepare(QSL("SELECT * FROM Categories WHERE account_id = :account_id;"));
  query_categories.bindValue(QSL(":account_id"), account_id);

  if (!query_categories.exec()) {
    qFatal("Query for obtaining categories failed. Error message: '%s'.",
           qPrintable(query_categories.lastError().text()));

    if (ok != nullptr) {
      *ok = false;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = true;
    }
  }

  DatabaseFactory::logLastExecutedQuery(query_categories);

  while (query_categories.next()) {
    AssignmentItem pair;

    pair.first = query_categories.value(CAT_DB_PARENT_ID_INDEX).toInt();
    pair.second = new T();

    auto* cat = static_cast<Category*>(pair.second);

    cat->setId(query_categories.value(CAT_DB_ID_INDEX).toInt());
    cat->setSortOrder(query_categories.value(CAT_DB_ORDER_INDEX).toInt());
    cat->setCustomId(query_categories.value(CAT_DB_CUSTOM_ID_INDEX).toString());

    if (cat->customId().isEmpty()) {
      cat->setCustomId(QString::number(cat->id()));
    }

    cat->setTitle(query_categories.value(CAT_DB_TITLE_INDEX).toString());
    cat->setDescription(query_categories.value(CAT_DB_DESCRIPTION_INDEX).toString());
    cat->setCreationDate(TextFactory::parseDateTime(query_categories.value(CAT_DB_DCREATED_INDEX).value<qint64>()));
    cat->setIcon(qApp->icons()->fromByteArray(query_categories.value(CAT_DB_ICON_INDEX).toByteArray()));

    categories << pair;
  }

  return categories;
}

template <typename T>
Assignment DatabaseQueries::getFeeds(const QSqlDatabase& db,
                                     const QList<MessageFilter*>& global_filters,
                                     int account_id,
                                     bool* ok) {
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

  DatabaseFactory::logLastExecutedQuery(query);

  while (query.next()) {
    AssignmentItem pair;

    pair.first = query.value(FDS_DB_CATEGORY_INDEX).toInt();

    Feed* feed = new T();

    // Load common data.
    feed->setTitle(query.value(FDS_DB_TITLE_INDEX).toString());
    feed->setId(query.value(FDS_DB_ID_INDEX).toInt());
    feed->setSortOrder(query.value(FDS_DB_ORDER_INDEX).toInt());
    feed->setSource(query.value(FDS_DB_SOURCE_INDEX).toString());
    feed->setCustomId(query.value(FDS_DB_CUSTOM_ID_INDEX).toString());

    if (feed->customId().isEmpty()) {
      feed->setCustomId(QString::number(feed->id()));
    }

    feed->setDescription(QString::fromUtf8(query.value(FDS_DB_DESCRIPTION_INDEX).toByteArray()));
    feed->setCreationDate(TextFactory::parseDateTime(query.value(FDS_DB_DCREATED_INDEX).value<qint64>()));
    feed->setIcon(qApp->icons()->fromByteArray(query.value(FDS_DB_ICON_INDEX).toByteArray()));
    feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(query.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
    feed->setAutoUpdateInterval(query.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
    feed->setIsSwitchedOff(query.value(FDS_DB_IS_OFF_INDEX).toBool());
    feed->setIsQuiet(query.value(FDS_DB_IS_QUIET_INDEX).toBool());
    feed->setRtlBehavior(query.value(FDS_DB_IS_RTL_INDEX).value<RtlBehavior>());

    Feed::ArticleIgnoreLimit art;

    art.m_addAnyArticlesToDb = query.value(FDS_DB_ADD_ANY_DATETIME_ARTICLES_INDEX).toBool();

    qint64 time_to_avoid = query.value(FDS_DB_DATETIME_TO_AVOID_INDEX).value<qint64>();

    if (time_to_avoid > 10000) {
      art.m_dtToAvoid = TextFactory::parseDateTime(time_to_avoid);
    }
    else {
      art.m_hoursToAvoid = time_to_avoid;
    }

    art.m_customizeLimitting = query.value(FDS_DB_KEEP_CUSTOMIZE).toBool();
    art.m_keepCountOfArticles = query.value(FDS_DB_KEEP_ARTICLES_COUNT).toInt();
    art.m_doNotRemoveUnread = query.value(FDS_DB_KEEP_UNREAD_ARTICLES).toBool();
    art.m_doNotRemoveStarred = query.value(FDS_DB_KEEP_STARRED_ARTICLES).toBool();
    art.m_moveToBinDontPurge = query.value(FDS_DB_RECYCLE_ARTICLES).toBool();

    feed->setArticleIgnoreLimit(art);

    qDebugNN << LOGSEC_CORE << "Custom ID of feed when loading from DB is" << QUOTE_W_SPACE_DOT(feed->customId());

    // Load custom data.
    feed->setCustomDatabaseData(deserializeCustomData(query.value(FDS_DB_CUSTOM_DATA_INDEX).toString()));

    if (filters_in_feeds.contains(feed->id())) {
      auto all_filters_for_this_feed = filters_in_feeds.values(feed->id());

      for (MessageFilter* fltr : global_filters) {
        if (all_filters_for_this_feed.contains(fltr->id())) {
          feed->appendMessageFilter(fltr);
        }
      }
    }

    pair.second = feed;

    feeds << pair;
  }

  return feeds;
}

template <typename Categ, typename Fee>
void DatabaseQueries::loadRootFromDatabase(ServiceRoot* root) {
  QSqlDatabase database = qApp->database()->driver()->connection(root->metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Categ>(database, root->accountId());
  Assignment feeds = DatabaseQueries::getFeeds<Fee>(database, qApp->feedReader()->messageFilters(), root->accountId());
  auto labels = DatabaseQueries::getLabelsForAccount(database, root->accountId());
  auto probes = DatabaseQueries::getProbesForAccount(database, root->accountId());

  root->performInitialAssembly(categories, feeds, labels, probes);
}

#endif // DATABASEQUERIES_H
