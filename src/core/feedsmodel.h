#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QAbstractItemModel>

#include "core/messagesmodel.h"
#include "core/feedsmodelrootitem.h"

#include <QIcon>


class FeedsModelCategory;
class FeedsModelFeed;
class FeedsModelStandardCategory;
class FeedsModelStandardFeed;

typedef QList<QPair<int, FeedsModelCategory*> > CategoryAssignment;
typedef QPair<int, FeedsModelCategory*> CategoryAssignmentItem;
typedef QList<QPair<int, FeedsModelFeed*> > FeedAssignment;
typedef QPair<int, FeedsModelFeed*> FeedAssignmentItem;

class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsModel(QObject *parent = 0);
    virtual ~FeedsModel();

    // Model implementation.
    inline QVariant data(const QModelIndex &index, int role) const {
      return itemForIndex(index)->data(index.column(), role);
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;

    // Returns couns of ALL/UNREAD (non-deleted) messages for the model.
    inline int countOfAllMessages() const {
      return m_rootItem->countOfAllMessages();
    }

    inline int countOfUnreadMessages() const {
      return m_rootItem->countOfUnreadMessages();
    }

    // Base manipulators.
    bool removeItem(const QModelIndex &index);

    // Standard category manipulators.
    bool addStandardCategory(FeedsModelStandardCategory *category,
                             FeedsModelRootItem *parent);
    bool editStandardCategory(FeedsModelStandardCategory *original_category,
                              FeedsModelStandardCategory *new_category);

    // Standard feed manipulators.
    /*
    bool addStandardFeed(FeedsModelStandardFeed *feed,
                         FeedsModelRootItem *parent);
    bool removeStandardFeed(FeedsModelStandardFeed *feed);
    */

    // Returns (undeleted) messages for given feeds.
    QList<Message> messagesForFeeds(const QList<FeedsModelFeed*> &feeds);

    // Returns all categories, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, FeedsModelCategory*> allCategories();

    // Returns categories from the subtree with given root node, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, FeedsModelCategory*> categoriesForItem(FeedsModelRootItem *root);

    // Returns list of all feeds contained in the model.
    QList<FeedsModelFeed*> allFeeds();

    // Get list of feeds from tree with particular item
    // as root. If root itself is a feed, then it is returned.
    QList<FeedsModelFeed*> feedsForItem(FeedsModelRootItem *root);

    // Returns list of feeds which belong to given indexes.
    // NOTE: If index is "category", then all child feeds are contained in the
    // result.
    // NOTE: This is particularly useful for displaying messages of
    // selected feeds.
    QList<FeedsModelFeed*> feedsForIndexes(const QModelIndexList &indexes);

    // Returns ALL CHILD feeds contained within single index.
    QList<FeedsModelFeed*> feedsForIndex(const QModelIndex &index);

    // Returns pointer to feed if it lies in given index
    // or NULL if no feed lies in given index.
    FeedsModelFeed *feedForIndex(const QModelIndex &index);

    // Returns pointer to category if it lies in given index
    // or NULL if no category lies in given index.
    FeedsModelCategory *categoryForIndex(const QModelIndex &index) const;

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    FeedsModelRootItem *itemForIndex(const QModelIndex &index) const;

    // Returns QModelIndex on which lies given item.
    QModelIndex indexForItem(FeedsModelRootItem *item) const;

    // Access to root item.
    inline FeedsModelRootItem *rootItem() const {
      return m_rootItem;
    }

  public slots:
    // Feeds operations.
    bool markFeedsRead(const QList<FeedsModelFeed*> &feeds, int read);
    bool markFeedsDeleted(const QList<FeedsModelFeed*> &feeds, int deleted);

    // Signals that properties (probably counts)
    // of ALL items have changed.
    void reloadWholeLayout();

    // Signals that SOME data of this model need
    // to be reloaded by ALL attached views.
    // NOTE: This reloads all parent valid indexes too.
    void reloadChangedLayout(QModelIndexList list);

  protected:
    // Returns converted ids of given feeds
    // which are suitable as IN clause for SQL queries.
    QStringList textualFeedIds(const QList<FeedsModelFeed*> &feeds);

    // Loads feed/categories from the database.
    void loadFromDatabase();

    // Takes lists of feeds/categories and assembles
    // them into the tree structure.
    void assembleCategories(CategoryAssignment categories);
    void assembleFeeds(FeedAssignment feeds);

  private:
    FeedsModelRootItem *m_rootItem;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;
};

#endif // FEEDSMODEL_H
