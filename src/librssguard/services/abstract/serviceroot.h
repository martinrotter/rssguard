// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SERVICEROOT_H
#define SERVICEROOT_H

#include "core/feedsmodel.h"
#include "core/message.h"
#include "definitions/typedefs.h"
#include "services/abstract/rootitem.h"

#include <QJsonDocument>
#include <QNetworkProxy>
#include <QPair>

class QAction;
class QMutex;
class RecycleBin;
class ImportantNode;
class UnreadNode;
class SearchsNode;
class LabelsNode;
class Label;
class MessagesModel;
class CustomMessagePreviewer;
class CacheForServiceRoot;
class FormAccountDetails;

// THIS IS the root node of the service.
// NOTE: The root usually contains some core functionality of the
// service like service account username/password etc.
class RSSGUARD_DLLSPEC ServiceRoot : public RootItem {
    Q_OBJECT

  public:
    enum class LabelOperation {
      Adding = 1,
      Editing = 2,
      Deleting = 4,

      // NOTE: Service fetches list of labels from remote source
      // and does not use local offline labels.
      Synchronised = 8
    };

    enum class BagOfMessages {
      Read,
      Unread,
      Starred
    };

  public:
    explicit ServiceRoot(RootItem* parent = nullptr);
    virtual ~ServiceRoot();

    // These methods bellow are part of "interface".
    RecycleBin* recycleBin() const;
    ImportantNode* importantNode() const;
    LabelsNode* labelsNode() const;
    SearchsNode* probesNode() const;
    UnreadNode* unreadNode() const;

    virtual FormAccountDetails* accountSetupDialog() const;
    virtual void onDatabaseCleanup();
    virtual void updateCounts(bool including_total_count);
    virtual bool canBeDeleted() const;
    virtual void deleteItem();
    virtual void editItems(const QList<RootItem*>& items);
    virtual void markAsReadUnread(ReadStatus status);
    virtual void cleanMessages(bool clean_read_only);
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual LabelOperation supportedLabelOperations() const;
    virtual QString additionalTooltip() const;
    virtual void saveAccountDataToDatabase();
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual bool wantsBaggedIdsOfExistingMessages() const;
    virtual bool displaysEnclosures() const;
    virtual void aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                          const QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>>&
                                            stated_messages,
                                          const QHash<QString, QStringList>& tagged_messages);

    // Returns list of specific actions for "Add new item" main window menu.
    // So typical list of returned actions could look like:
    //  a) Add new feed
    //  b) Add new category
    //  c) ...
    // NOTE: Caller does NOT take ownership of created menu/actions!
    virtual QList<QAction*> addItemMenu();

    // NOTE: Caller does NOT take ownership of created menu/actions!
    virtual QList<QAction*> contextMenuFeedsList();

    // NOTE: Caller does NOT take ownership of created menu/actions!
    virtual QList<QAction*> contextMenuMessagesList(const QList<Message>& messages);

    // Returns list of specific actions to be shown in main window menu
    // bar in sections "Services -> 'this service'".
    // NOTE: Caller does NOT take ownership of created menu!
    virtual QList<QAction*> serviceMenu();

    // If plugin uses online synchronization of feeds/labels/etc, then returns true.
    virtual bool isSyncable() const;

    // Start/stop services.
    // Start method is called when feed model gets initialized OR after user adds new service.
    // Account should synchronously initialize its children (load them from DB is recommended
    // here).
    //
    // Stop method is called just before application exits OR when
    // user explicitly deletes existing service instance.
    virtual void start(bool freshly_activated);
    virtual void stop();

    // Obtains list of messages.
    // Throws exception subclassed from ApplicationException, preferably FeedFetchException
    // if any problems arise.
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages) = 0;

    // Returns special widget to display articles of this account type.
    // Caller does NOT free returned previewer after usage from memory,
    // it only may hide it.
    // Thus, account is responsible to free any custom previewers.
    virtual CustomMessagePreviewer* customMessagePreviewer();

    // This method should load messages for given "item" into model.
    virtual bool loadMessagesForItem(RootItem* item, MessagesModel* model);

    // These are called BEFORE and AFTER this article status update is stored in DB.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.

    virtual void onBeforeSetMessagesRead(RootItem* selected_item,
                                         const QStringList& message_custom_ids,
                                         ReadStatus read);
    virtual void onBeforeSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);
    virtual void onAfterSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);

    virtual void onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);
    virtual void onAfterSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

    virtual void onBeforeMessagesDelete(RootItem* selected_item, const QList<Message>& messages);
    virtual void onAfterMessagesDelete(RootItem* selected_item, const QList<Message>& messages);

    virtual void onBeforeLabelMessageAssignmentChanged(const QList<Label*>& labels,
                                                       const QList<Message>& messages,
                                                       bool assign);
    virtual void onAfterLabelMessageAssignmentChanged(const QList<Label*>& labels,
                                                      const QList<Message>& messages,
                                                      bool assign);

    virtual void onBeforeMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);
    virtual void onAfterMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

    virtual void onAfterFeedsPurged(const QList<Feed*>& feeds);

    // Returns the UNIQUE code of the given service.
    // NOTE: Keep in sync with ServiceEntryRoot::code().
    virtual QString code() const = 0;

    // Provides abstraction for service-wide or feed-wide proxy.
    virtual QNetworkProxy networkProxyForItem(RootItem* item) const;

  public:
    CacheForServiceRoot* toCache() const;

    // Account ID corresponds with DB attribute Accounts (id).
    int accountId() const;
    void setAccountId(int account_id);

    QNetworkProxy networkProxy() const;
    void setNetworkProxy(const QNetworkProxy& network_proxy);

    // Removes all data associated with this account from DB
    // and from model.
    void completelyRemoveAllData();

    // Returns counts of updated messages <unread, all>.
    UpdatedArticles updateMessages(QList<Message>& messages,
                                   Feed* feed,
                                   bool force_update,
                                   bool recalculate_counts,
                                   QMutex* db_mutex);

    // Removes all/read only messages from given underlying feeds.
    void cleanFeeds(const QList<Feed*>& items, bool clean_read_only);

    // Obvious methods to wrap signals.
    void itemChanged(const QList<RootItem*>& items);
    void informOthersAboutDataChange(RootItem* item, FeedsModel::ExternalDataChange change);
    void requestItemExpand(const QList<RootItem*>& items, bool expand);
    void requestItemExpandStateSave(RootItem* subtree_root);
    void requestItemReassignment(RootItem* item, RootItem* new_parent, bool blocking = false);
    void requestItemsReassignment(const QList<RootItem*>& items, RootItem* new_parent);
    void requestItemRemoval(RootItem* item);

    // Some message/feed attribute selectors.
    QStringList textualFeedIds(const QList<Feed*>& feeds) const;
    QStringList customIDsOfMessages(const QList<ImportanceChange>& changes);
    QStringList customIDsOfMessages(const QList<Message>& messages);

    // Returns list of article IDs depending on what target operation is.
    // NOTE: So if we want to mark some articles as read,
    // then we only return UNREAD IDs here to really return
    // only IDs when the change makes sense.
    // NOTE: Importance is not dealt here because it was not needed
    // yet.
    QStringList customIDsOfMessagesForItem(RootItem* item,
                                           RootItem::ReadStatus target_read = RootItem::ReadStatus::Unknown);

    void performInitialAssembly(const Assignment& categories,
                                const Assignment& feeds,
                                const QList<Label*>& labels,
                                const QList<Search*>& probes);

    bool nodeShowUnread() const;
    void setNodeShowUnread(bool enabled);

    bool nodeShowImportant() const;
    void setNodeShowImportant(bool enabled);

    bool nodeShowLabels() const;
    void setNodeShowLabels(bool enabled);

    bool nodeShowProbes() const;
    void setNodeShowProbes(bool enabled);

  public slots:
    virtual void addNewFeed(RootItem* selected_item, const QString& url = QString());
    virtual void addNewCategory(RootItem* selected_item);
    virtual void syncIn();

  protected:
    // This method should obtain new tree of feed/categories/whatever to perform sync in.
    virtual RootItem* obtainNewTreeForSyncIn() const;

    // Removes all messages/categories/feeds which are
    // associated with this account.
    void removeOldAccountFromDatabase(bool delete_messages_too, bool delete_labels_too);
    void cleanAllItemsFromModel(bool clean_labels_too);
    void appendCommonNodes();

    // Takes lists of feeds/categories and assembles them into the tree structure.
    void assembleCategories(const Assignment& categories);
    void assembleFeeds(const Assignment& feeds);

  signals:
    void proxyChanged(QNetworkProxy proxy);
    void dataChanged(QList<RootItem*> items);
    void dataChangeNotificationTriggered(RootItem* item, FeedsModel::ExternalDataChange change);
    void itemExpandRequested(QList<RootItem*> items, bool expand);
    void itemExpandStateSaveRequested(RootItem* subtree_root);

    void itemBlockingReassignmentRequested(RootItem* item, RootItem* new_parent);
    void itemReassignmentRequested(RootItem* item, RootItem* new_parent);
    void itemRemovalRequested(RootItem* item);

  private:
    void refreshAfterArticlesChange(const QList<Message>& messages,
                                    bool refresh_bin,
                                    bool refresh_only_bin,
                                    bool including_total_counts);

    void resortAccountTree(RootItem* tree,
                           const QMap<QString, QVariantMap>& custom_category_data,
                           const QMap<QString, QVariantMap>& custom_feed_data) const;

    // Key is feed's custom ID.
    virtual QMap<QString, QVariantMap> storeCustomFeedsData();

    // Key is category's custom ID.
    virtual QMap<QString, QVariantMap> storeCustomCategoriesData();

    // Key is label's custom ID.
    virtual QMap<QString, QVariantMap> storeCustomLabelsData();

    virtual void restoreCustomFeedsData(const QMap<QString, QVariantMap>& data, const QHash<QString, Feed*>& feeds);
    virtual void restoreCustomCategoriesData(const QMap<QString, QVariantMap>& data,
                                             const QHash<QString, Category*>& cats);
    virtual void restoreCustomLabelsData(const QMap<QString, QVariantMap>& data, LabelsNode* labels);

  protected:
    RecycleBin* m_recycleBin;
    ImportantNode* m_importantNode;
    LabelsNode* m_labelsNode;
    SearchsNode* m_probesNode;
    UnreadNode* m_unreadNode;
    int m_accountId;
    QList<QAction*> m_serviceMenu;
    QNetworkProxy m_networkProxy;
    bool m_nodeShowUnread;
    bool m_nodeShowImportant;
    bool m_nodeShowLabels;
    bool m_nodeShowProbes;
};

#if QT_VERSION_MAJOR == 6
inline size_t qHash(ServiceRoot::BagOfMessages key, size_t seed) {
  return ::qHash(static_cast<uint>(key), seed);
}

#else
inline uint qHash(ServiceRoot::BagOfMessages key, uint seed) {
  return ::qHash(static_cast<uint>(key), seed);
}

#endif

ServiceRoot::LabelOperation operator|(ServiceRoot::LabelOperation lhs, ServiceRoot::LabelOperation rhs);
ServiceRoot::LabelOperation operator&(ServiceRoot::LabelOperation lhs, ServiceRoot::LabelOperation rhs);

#endif // SERVICEROOT_H
