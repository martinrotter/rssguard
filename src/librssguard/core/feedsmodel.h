// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include "services/abstract/rootitem.h"

#include <QAbstractItemModel>

class Category;
class Feed;
class ServiceRoot;
class ServiceEntryPoint;
class StandardServiceRoot;

class RSSGUARD_DLLSPEC FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    enum class ExternalDataChange {
      MarkedReadUnread,
      DatabaseCleaned,
      RecycleBinRestored,
      AccountSyncedIn,
      FeedFetchFinished,
      ListFilterChanged
    };

    explicit FeedsModel(QObject* parent = nullptr);
    virtual ~FeedsModel();

    // Model implementation.
    virtual QVariant data(const QModelIndex& index, int role) const;

    // Drag & drop.
    virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

    // Other subclassed methods.
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual int rowCount(const QModelIndex& parent) const;

    // Returns counts of ALL/UNREAD (non-deleted) messages for the model.
    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    // Returns all activated service roots.
    // NOTE: Service root nodes are lying directly UNDER
    // the model root item.
    QList<ServiceRoot*> serviceRoots() const;

    // Returns the list of feeds which should be updated
    // according to auto-update schedule.
    // Variable "auto_update_now" is true, when global timeout
    // for scheduled auto-update was met and global auto-update strategy is enabled
    // so feeds with "default" auto-update strategy should be updated.
    //
    // This method might change some properties of some feeds.
    QList<Feed*> feedsForScheduledUpdate(bool auto_update_now);

    // Returns ALL RECURSIVE CHILD feeds contained within single index.
    QList<Feed*> feedsForIndex(const QModelIndex& index = QModelIndex()) const;

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    RootItem* itemForIndex(const QModelIndex& index) const;

    // Returns source QModelIndex on which lies given item.
    // NOTE: This goes through all available indexes and
    // checks their bound items manually, there is no
    // other way to to this.
    QModelIndex indexForItem(const RootItem* item) const;

    // Determines if any feed has any new messages.
    bool hasAnyFeedNewMessages() const;

    // Access to root item.
    RootItem* rootItem() const;

    void setupBehaviorDuringFetching();
    void setupFonts();
    void informAboutDatabaseCleanup();

  public slots:
    void loadActivatedServiceAccounts();

    // Stops all accounts before exit.
    void stopServiceAccounts();

    // Reloads counts of all feeds/categories/whatever in the model.
    void reloadCountsOfWholeModel();

    // Checks if new parent node is different from one used by original node.
    // If it is, then it reassigns original_node to new parent.
    void reassignNodeToNewParent(RootItem* original_node, RootItem* new_parent);

    // Adds given service root account.
    bool addServiceAccount(ServiceRoot* root, bool freshly_activated);

    // Removes item with given index.
    // NOTE: Also deletes item from memory.
    void removeItem(const QModelIndex& index);
    void removeItem(RootItem* deleting_item);

    // Recycle bins operations.
    bool restoreAllBins();
    bool emptyAllBins();

    void changeSortOrder(RootItem* item, bool move_top, bool move_bottom, int new_sort_order = {});

    // Takes direct descendants (but only categories or feeds)
    // and rearranges them alphabetically.
    void sortDirectDescendants(RootItem* item, RootItem::Kind kind_to_sort);

    // Feeds operations.
    bool markItemRead(RootItem* item, RootItem::ReadStatus read);
    bool markItemCleared(RootItem* item, bool clean_read_only);
    bool purgeArticles(const QList<Feed*>& feeds);

    // Signals that properties (probably counts)
    // of ALL items have changed.
    void reloadWholeLayout();

    // Signals that SOME data of this model need
    // to be reloaded by ALL attached views.
    // NOTE: This reloads all parent valid indexes too.
    void reloadChangedLayout(QModelIndexList list);

    // Invalidates data under index for the item.
    void reloadChangedItem(RootItem* item);

    // Notifies other components about messages
    // counts.
    void notifyWithCounts();

  private slots:
    void onItemDataChanged(const QList<RootItem*>& items);

  signals:
    void messageCountsChanged(int unread_messages, bool any_feed_has_unread_messages);

    // Emitted if any item requested that any view should expand it.
    void itemExpandRequested(QList<RootItem*> items, bool expand);

    // Emitted if any item requested that its expand states should be explicitly saved.
    // NOTE: Normally expand states are saved when application quits.
    void itemExpandStateSaveRequested(RootItem* subtree_root);

    // Emitted to notify article model about external data change.
    // Article model will reload article, refresh selected article etc.
    void dataChangeNotificationTriggered(ExternalDataChange change);

  private:
    bool m_updateDuringFetching;
    QIcon m_updateItemIcon;
    RootItem* m_rootItem;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;
    QFont m_normalFont;
    QFont m_boldFont;
    QFont m_normalStrikedFont;
    QFont m_boldStrikedFont;
};

#endif // FEEDSMODEL_H
