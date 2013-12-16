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

    // Returns all categories.
    QHash<int, FeedsModelCategory*> getCategories();

    // Returns categories from the subtree with given root node.
    QHash<int, FeedsModelCategory*> getCategories(FeedsModelRootItem *root);

    // Returns list of feeds which belong to given indexes.
    // NOTE: If index is "category", then all child feeds are contained in the
    // result.
    // NOTE: This is particularly useful for displaying messages of selected feeds.
    QList<FeedsModelFeed*> feedsForIndexes(const QModelIndexList &indexes);

    // Returns feeds contained within single index.
    QList<FeedsModelFeed*> feedsForIndex(const QModelIndex &index);

  protected:
    // Loads feed/categories from the database.
    void loadFromDatabase();

    // TODO: Otestovat metody itemForIndex, feedsForIndex, feedsForIndexes.

    // Returns feed/category which lies at the specified index.
    FeedsModelRootItem *itemForIndex(const QModelIndex &index);

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
