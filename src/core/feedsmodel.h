#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QAbstractItemModel>
#include <QIcon>


class FeedsModelRootItem;
class FeedsModelCategory;
class FeedsModelFeed;

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
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;

    // Returns all categories, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, FeedsModelCategory*> getAllCategories();

    // Returns categories from the subtree with given root node, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, FeedsModelCategory*> getCategories(FeedsModelRootItem *root);

    // Returns list of all feeds contained in the model.
    QList<FeedsModelFeed*> getAllFeeds();

    // Get list of feeds from tree with particular item
    // as root. If root itself is a feed, then it is returned.
    QList<FeedsModelFeed*> getFeeds(FeedsModelRootItem *root);

    // Returns list of feeds which belong to given indexes.
    // NOTE: If index is "category", then all child feeds are contained in the
    // result.
    // NOTE: This is particularly useful for displaying messages of selected feeds.
    QList<FeedsModelFeed*> feedsForIndexes(const QModelIndexList &indexes);

    // Returns feeds contained within single index.
    QList<FeedsModelFeed*> feedsForIndex(const QModelIndex &index);

    static bool isUnequal(FeedsModelFeed *lhs, FeedsModelFeed *rhs);
    static bool isEqual(FeedsModelFeed *lhs, FeedsModelFeed *rhs);

  public slots:
    // Signals that properties (probably counts)
    // of ALL items have changed.
    void reloadWholeLayout();

    // Signals that SOME data of this model need
    // to be reloaded by ALL attached views.
    void reloadChangedLayout(QModelIndexList list);

  protected:
    // Returns feed/category which lies at the specified index or
    // null if index is invalid.
    FeedsModelRootItem *itemForIndex(const QModelIndex &index) const;

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
