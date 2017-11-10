// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SERVICEROOT_H
#define SERVICEROOT_H

#include "services/abstract/rootitem.h"

#include "core/message.h"

#include <QPair>

class FeedsModel;
class RecycleBin;
class QAction;
class MessagesModel;

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
    explicit ServiceRoot(RootItem* parent = nullptr);
    virtual ~ServiceRoot();

    void updateCounts(bool including_total_count);
    bool deleteViaGui();
    bool markAsReadUnread(ReadStatus status);

    virtual RecycleBin* recycleBin() const;

    void setRecycleBin(RecycleBin* recycle_bin);

    virtual bool downloadAttachmentOnMyOwn(const QUrl& url) const;

    QList<Message> undeletedMessages() const;
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;

    // Returns list of specific actions for "Add new item" main window menu.
    // So typical list of returned actions could look like:
    //  a) Add new feed
    //  b) Add new category
    //  c) ...
    // NOTE: Caller does NOT take ownership of created menu!
    virtual QList<QAction*> addItemMenu();

    // Returns actions to display as context menu.
    QList<QAction*> contextMenu();

    // Returns list of specific actions to be shown in main window menu
    // bar in sections "Services -> 'this service'".
    // NOTE: Caller does NOT take ownership of created menu!
    virtual QList<QAction*> serviceMenu();

    // Start/stop services.
    // Start method is called when feed model gets initialized OR after user adds new service.
    // Account should synchronously initialize its children (load them from DB is recommended
    // here).
    //
    // Stop method is called just before application exits OR when
    // user explicitly deletes existing service instance.
    virtual void start(bool freshly_activated);
    virtual void stop();

    // Account ID corresponds with DB attribute Accounts (id).
    int accountId() const;
    void setAccountId(int account_id);

    // Returns the UNIQUE code of the given service.
    // NOTE: Keep in sync with ServiceEntryRoot::code().
    virtual QString code() const = 0;

    // Removes all/read only messages from given underlying feeds.
    bool cleanFeeds(QList<Feed*> items, bool clean_read_only);

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

    // Called BEFORE the list of messages is about to be restored from recycle bin
    // by the user from message list.
    // Selected item is naturally recycle bin.
    virtual bool onBeforeMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

    // Called AFTER the list of messages was restored from recycle bin
    // by the user from message list.
    // Selected item is naturally recycle bin.
    virtual bool onAfterMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

    void completelyRemoveAllData();
    QStringList customIDSOfMessagesForItem(RootItem* item);
    bool markFeedsReadUnread(QList<Feed*> items, ReadStatus read);

    // Obvious methods to wrap signals.
    void itemChanged(const QList<RootItem*>& items);
    void requestReloadMessageList(bool mark_selected_messages_read);
    void requestItemExpand(const QList<RootItem*>& items, bool expand);
    void requestItemExpandStateSave(RootItem* subtree_root);
    void requestItemReassignment(RootItem* item, RootItem* new_parent);
    void requestItemRemoval(RootItem* item);

  public slots:
    virtual void addNewFeed(const QString& url = QString());
    virtual void addNewCategory();
    virtual void syncIn();

  protected:

    // This method should obtain new tree of feed/messages/etc to perform
    // sync in.
    virtual RootItem* obtainNewTreeForSyncIn() const;

    // Removes all messages/categories/feeds which are
    // associated with this account.
    void removeOldFeedTree(bool including_messages);
    void storeNewFeedTree(RootItem* root);
    void cleanAllItems();

    // Removes messages which do not belong to any
    // existing feed.
    //
    // NOTE: This situation may happen if user deletes some feed
    // from another machine and then performs sync-in on this machine.
    void removeLeftOverMessages();

    QStringList textualFeedUrls(const QList<Feed*>& feeds) const;
    QStringList textualFeedIds(const QList<Feed*>& feeds) const;
    QStringList customIDsOfMessages(const QList<ImportanceChange>& changes);
    QStringList customIDsOfMessages(const QList<Message>& messages);

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
    virtual QMap<QString, QVariant> storeCustomFeedsData();
    virtual void restoreCustomFeedsData(const QMap<QString, QVariant>& data, const QHash<QString, Feed*>& feeds);

  private:
    RecycleBin* m_recycleBin;
    int m_accountId;
};

#endif // SERVICEROOT_H
