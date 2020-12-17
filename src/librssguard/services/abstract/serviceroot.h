// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SERVICEROOT_H
#define SERVICEROOT_H

#include "services/abstract/rootitem.h"

#include "core/message.h"

#include <QPair>

class FeedsModel;
class RecycleBin;
class ImportantNode;
class LabelsNode;
class Label;
class QAction;
class MessagesModel;
class CacheForServiceRoot;

// Car here represents ID (int, primary key) of the item.
typedef QList<QPair<int, RootItem*>> Assignment;
typedef QPair<int, RootItem*> AssignmentItem;
typedef QPair<Message, RootItem::Importance> ImportanceChange;

// THIS IS the root node of the service.
// NOTE: The root usually contains some core functionality of the
// service like service account username/password etc.
class ServiceRoot : public RootItem {
  Q_OBJECT

  public:
    enum class LabelOperation {
      Adding = 1,
      Editing = 2,
      Deleting = 4
    };

  public:
    explicit ServiceRoot(RootItem* parent = nullptr);
    virtual ~ServiceRoot();

    // These methods bellow are part of "interface".
    virtual void updateCounts(bool including_total_count);
    virtual bool deleteViaGui();
    virtual bool markAsReadUnread(ReadStatus status);
    virtual RecycleBin* recycleBin() const;
    virtual ImportantNode* importantNode() const;
    virtual LabelsNode* labelsNode() const;
    virtual bool downloadAttachmentOnMyOwn(const QUrl& url) const;
    virtual QList<Message> undeletedMessages() const;
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual LabelOperation supportedLabelOperations() const;

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

    // This method should prepare messages for given "item" (download them maybe?)
    // into predefined "Messages" table
    // and then use method QSqlTableModel::setFilter(....).
    // NOTE: It would be more preferable if all messages are downloaded
    // right when feeds are updated.
    virtual bool loadMessagesForItem(RootItem* item, MessagesModel* model);

    // Called BEFORE this read status update (triggered by user in message list) is stored in DB,
    // when false is returned, change is aborted.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.
    //
    // "read" is status which is ABOUT TO BE SET.
    virtual bool onBeforeSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);

    // Called AFTER this read status update (triggered by user in message list) is stored in DB,
    // when false is returned, change is aborted.
    // Here service root should inform (via signals)
    // which items are actually changed.
    //
    // "read" is status which is ABOUT TO BE SET.
    virtual bool onAfterSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);

    // Called BEFORE this importance switch update is stored in DB,
    // when false is returned, change is aborted.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.
    //
    // "changes" - list of pairs - <message (integer id), new status>
    virtual bool onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

    // Called AFTER this importance switch update is stored in DB,
    // when false is returned, change is aborted.
    // Here service root should inform (via signals)
    // which items are actually changed.
    //
    // "changes" - list of pairs - <message (integer id), new status>
    virtual bool onAfterSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

    // Called BEFORE the list of messages is about to be deleted
    // by the user from message list.
    virtual bool onBeforeMessagesDelete(RootItem* selected_item, const QList<Message>& messages);

    // Called AFTER the list of messages was deleted
    // by the user from message list.
    virtual bool onAfterMessagesDelete(RootItem* selected_item, const QList<Message>& messages);

    // Called BEFORE some labels are assigned/deassigned from/to messages.
    virtual bool onBeforeLabelMessageAssignmentChanged(const QList<Label*> labels, const QList<Message>& messages, bool assign);

    // Called AFTER some labels are assigned/deassigned from/to messages.
    virtual bool onAfterLabelMessageAssignmentChanged(const QList<Label*> labels, const QList<Message>& messages, bool assign);

    // Called BEFORE the list of messages is about to be restored from recycle bin
    // by the user from message list.
    // Selected item is naturally recycle bin.
    virtual bool onBeforeMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

    // Called AFTER the list of messages was restored from recycle bin
    // by the user from message list.
    // Selected item is naturally recycle bin.
    virtual bool onAfterMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

    // Returns the UNIQUE code of the given service.
    // NOTE: Keep in sync with ServiceEntryRoot::code().
    virtual QString code() const = 0;

    // These are not part of "interface".
    CacheForServiceRoot* toCache() const;

  public:

    // Account ID corresponds with DB attribute Accounts (id).
    int accountId() const;
    void setAccountId(int account_id);

    // Removes all data associated with this account from DB
    // and from model.
    void completelyRemoveAllData();

    // Removes all/read only messages from given underlying feeds.
    bool cleanFeeds(QList<Feed*> items, bool clean_read_only);

    // Marks all messages from feeds read/unread.
    bool markFeedsReadUnread(QList<Feed*> items, ReadStatus read);

    // Obvious methods to wrap signals.
    void itemChanged(const QList<RootItem*>& items);
    void requestReloadMessageList(bool mark_selected_messages_read);
    void requestItemExpand(const QList<RootItem*>& items, bool expand);
    void requestItemExpandStateSave(RootItem* subtree_root);
    void requestItemReassignment(RootItem* item, RootItem* new_parent);
    void requestItemRemoval(RootItem* item);

    // Some message/feed attribute selectors.
    QStringList textualFeedUrls(const QList<Feed*>& feeds) const;
    QStringList textualFeedIds(const QList<Feed*>& feeds) const;
    QStringList customIDsOfMessages(const QList<ImportanceChange>& changes);
    QStringList customIDsOfMessages(const QList<Message>& messages);
    QStringList customIDSOfMessagesForItem(RootItem* item);

  public slots:
    virtual void addNewFeed(RootItem* selected_item, const QString& url = QString());
    virtual void addNewCategory(RootItem* selected_item);
    virtual void syncIn();

  protected:
    void performInitialAssembly(const Assignment& categories, const Assignment& feeds, const QList<Label*>& labels);

    // This method should obtain new tree of feed/categories/whatever to perform sync in.
    virtual RootItem* obtainNewTreeForSyncIn() const;

    // Removes all messages/categories/feeds which are
    // associated with this account.
    void removeOldAccountFromDatabase(bool including_messages);
    void storeNewFeedTree(RootItem* root);
    void cleanAllItemsFromModel();

    // Removes messages which do not belong to any
    // existing feed.
    //
    // NOTE: This situation may happen if user deletes some feed
    // from another machine and then performs sync-in on this machine.
    void removeLeftOverMessages();

    // Removes all msg. filter assignments which are (within this account)
    // assigned to feed (via custom ID) which does not exist anymore.
    //
    // NOTE: This situation may happen if user deletes some feed
    // from another machine and then performs sync-in on this machine.
    void removeLeftOverMessageFilterAssignments();

    // Removes all labels/message assignments which are
    // assigned to non-existing messages or which are
    // assigned from non-existing labels.
    void removeLeftOverMessageLabelAssignments();

    // Takes lists of feeds/categories and assembles them into the tree structure.
    void assembleCategories(Assignment categories);
    void assembleFeeds(Assignment feeds);

  signals:
    void dataChanged(QList<RootItem*> items);
    void reloadMessageListRequested(bool mark_selected_messages_read);
    void itemExpandRequested(QList<RootItem*> items, bool expand);
    void itemExpandStateSaveRequested(RootItem* subtree_root);

    void itemReassignmentRequested(RootItem* item, RootItem* new_parent);
    void itemRemovalRequested(RootItem* item);

  private:
    virtual QMap<QString, QVariantMap> storeCustomFeedsData();
    virtual void restoreCustomFeedsData(const QMap<QString, QVariantMap>& data, const QHash<QString, Feed*>& feeds);

  protected:
    RecycleBin* m_recycleBin;
    ImportantNode* m_importantNode;
    LabelsNode* m_labelsNode;
    int m_accountId;
    QList<QAction*> m_serviceMenu;
};

ServiceRoot::LabelOperation operator|(ServiceRoot::LabelOperation lhs, ServiceRoot::LabelOperation rhs);
ServiceRoot::LabelOperation operator&(ServiceRoot::LabelOperation lhs, ServiceRoot::LabelOperation rhs);

#endif // SERVICEROOT_H
