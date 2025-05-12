// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ROOTITEM_H
#define ROOTITEM_H

#include "core/message.h"

#include <QDateTime>
#include <QFont>
#include <QIcon>

class Category;
class Feed;
class Label;
class Search;
class ServiceRoot;
class QAction;

// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all other non-root items of FeedsModel.
class RSSGUARD_DLLSPEC RootItem : public QObject {
    Q_OBJECT

    // Added for message filtering with labels.
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(QString customId READ customId)

  public:
    enum class ReadStatus {
      Unread = 0,
      Read = 1,
      Unknown = 256
    };

    // Holds statuses for messages
    // to be switched importance (starred).
    enum class Importance {
      NotImportant = 0,
      Important = 1,
      Unknown = 256
    };

    // Describes the kind of the item.
    enum class Kind {
      Root = 1,
      Bin = 2,
      Feed = 4,
      Category = 8,
      ServiceRoot = 16,
      Labels = 32,
      Important = 64,
      Label = 128,
      Unread = 256,
      Probes = 512,
      Probe = 1024
    };

    // Constructors and destructors.
    explicit RootItem(RootItem* parent_item = nullptr);
    explicit RootItem(const RootItem& other);
    virtual ~RootItem();

    // Determines if this item should be kept always in the beginning of feeds list.
    virtual QString hashCode() const;
    virtual QString additionalTooltip() const;

    // Returns list of specific actions which can be done with the item.
    // Do not include general actions here like actions: Mark as read, Update, ...
    // NOTE: Ownership of returned actions is not switched to caller, free them when needed.
    virtual QList<QAction*> contextMenuFeedsList();

    // Can properties of this item be edited?
    virtual bool canBeEdited() const;

    // Can the item be deleted?
    virtual bool canBeDeleted() const;

    // Performs deletion of the item, this
    // method should NOT display any additional dialogs.
    // Returns result status.
    virtual bool deleteItem();

    virtual bool isFetching() const;

    // Performs all needed steps (DB update, remote server update)
    // to mark this item as read/unread.
    virtual bool markAsReadUnread(ReadStatus status);

    // Get ALL undeleted messages from this item in one single list.
    // This is currently used for displaying items in "newspaper mode".
    virtual QList<Message> undeletedMessages() const;

    // This method should "clean" all messages it contains.
    //
    // NOTE: What "clean" means? It means delete messages -> move them to recycle bin
    // or eventually remove them completely if there is no recycle bin functionality.
    //
    // If this method is called on "recycle bin" instance of your
    // service account, it should "empty" the recycle bin.
    virtual bool cleanMessages(bool clear_only_read);

    // Reloads current counts of articles in this item from DB and
    // sets.
    virtual void updateCounts(bool including_total_count);
    virtual int row() const;
    virtual QVariant data(int column, int role) const;
    virtual Qt::ItemFlags additionalFlags() const;
    virtual bool performDragDropChange(RootItem* target_item);

    // Each item offers "counts" of messages.
    // Returns counts of messages of all child items summed up.
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

    RootItem* parent() const;
    void setParent(RootItem* parent_item);

    // Access to children.
    RootItem* child(int row);
    int childCount() const;
    void appendChild(RootItem* child);

    QList<RootItem*> childItems(RootItem::Kind kind) const;
    QList<RootItem*> childItems() const;
    QList<RootItem*>& childItems();

    void clearChildren();
    void setChildItems(const QList<RootItem*>& child_items);

    // Removes particular child at given index.
    // NOTE: Child is NOT freed from the memory.
    bool removeChild(int index);
    bool removeChild(RootItem* child);

    // Checks whether "this" object is child (direct or indirect)
    // of the given root.
    bool isChildOf(const RootItem* root) const;

    // Is "this" item parent (direct or indirect) if given child?
    bool isParentOf(const RootItem* child) const;

    // Returns flat list of all items from subtree where this item is a root.
    // Returned list includes this item too.
    QList<RootItem*> getSubTree() const;
    QList<RootItem*> getSubTree(std::function<bool(const RootItem*)> tester) const;
    QList<RootItem*> getSubTree(RootItem::Kind kind_of_item) const;
    QList<Category*> getSubTreeCategories() const;

    RootItem* getItemFromSubTree(std::function<bool(const RootItem*)> tester) const;

    // Returns list of categories complemented by their own integer primary ID.
    QHash<int, Category*> getSubTreeCategoriesForAssemble() const;

    // Returns list of categories complemented by their own string CUSTOM ID.
    QHash<QString, Category*> getHashedSubTreeCategories() const;

    // Returns list of feeds complemented by their own string CUSTOM ID.
    QHash<QString, Feed*> getHashedSubTreeFeeds() const;

    QList<Feed*> getSubTreeFeeds(bool recursive = true) const;
    QList<Feed*> getSubTreeAutoFetchingWithManualIntervalsFeeds() const;
    QList<Feed*> getSubAutoFetchingEnabledFeeds() const;

    // Returns the service root node which is direct or indirect parent of current item.
    ServiceRoot* getParentServiceRoot() const;

    RootItem::Kind kind() const;
    void setKind(RootItem::Kind kind);

    // Each item can have icon.
    QIcon icon() const;
    void setIcon(const QIcon& icon);

    // Returns icon, even if item has "default" icon set, then
    // this icon is extra loaded and returned.
    QIcon fullIcon() const;

    // This ALWAYS represents primary column number/ID under which
    // the item is stored in DB.
    int id() const;

    // WARNING: Do not EVER call this method if your "this" object is derived
    // from "ServiceRoot";
    void setId(int id);

    // Each item has its title.
    QString title() const;
    QString sanitizedTitle() const;
    void setTitle(const QString& title);

    // This should be in UTC and should be converted to localtime when needed.
    QDateTime creationDate() const;
    void setCreationDate(const QDateTime& creation_date);

    QString description() const;
    void setDescription(const QString& description);

    // NOTE: For standard feed/category, this WILL equal to id().
    QString customId() const;
    int customNumericId() const;
    void setCustomId(const QString& custom_id);

    // Converters
    Category* toCategory() const;
    Feed* toFeed() const;
    Label* toLabel() const;
    Search* toProbe() const;
    ServiceRoot* toServiceRoot() const;

    bool keepOnTop() const;
    void setKeepOnTop(bool keep_on_top);

    // Sort order, when items in feeds list are sorted manually.
    //
    // NOTE: This is only used for "Account", "Category" and "Feed" classes
    // which can be manually sorted. Other types like "Label" cannot be
    // automatically sorted and are always sorted by title.
    //
    // Sort order number cannot be negative but order of list of items with same
    // parent MUST form continuous series AND start with zero, for example:
    //   0, 1, 2, 3, 4, ...
    //
    // NOTE: This is checked with DatabaseQueries::fixupOrders() method on app startup.
    int sortOrder() const;
    void setSortOrder(int sort_order);

  private:
    RootItem::Kind m_kind;
    int m_id;
    QString m_customId;
    QString m_title;
    QString m_description;
    QIcon m_icon;
    QDateTime m_creationDate;
    bool m_keepOnTop;
    int m_sortOrder;
    QList<RootItem*> m_childItems;
    RootItem* m_parentItem;
};

inline RootItem* RootItem::parent() const {
  return m_parentItem;
}

inline void RootItem::setParent(RootItem* parent_item) {
  m_parentItem = parent_item;
}

inline RootItem* RootItem::child(int row) {
  return m_childItems.value(row);
}

inline int RootItem::childCount() const {
  return m_childItems.size();
}

inline void RootItem::appendChild(RootItem* child) {
  if (child != nullptr) {
    m_childItems.append(child);
    child->setParent(this);
  }
}

inline QList<RootItem*> RootItem::childItems() const {
  return m_childItems;
}

inline QList<RootItem*>& RootItem::childItems() {
  return m_childItems;
}

inline void RootItem::clearChildren() {
  m_childItems.clear();
}

inline void RootItem::setChildItems(const QList<RootItem*>& child_items) {
  clearChildren();

  for (RootItem* ch : child_items) {
    appendChild(ch);
  }
}

RootItem::Kind operator|(RootItem::Kind a, RootItem::Kind b);
RootItem::Kind operator&(RootItem::Kind a, RootItem::Kind b);

QDataStream& operator<<(QDataStream& out, const RootItem::Importance& myObj);
QDataStream& operator>>(QDataStream& in, RootItem::Importance& myObj);
QDataStream& operator<<(QDataStream& out, const RootItem::ReadStatus& myObj);
QDataStream& operator>>(QDataStream& in, RootItem::ReadStatus& myObj);

#endif // ROOTITEM_H
