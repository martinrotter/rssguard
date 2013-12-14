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

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;

    void loadFromDatabase();

    QHash<int, FeedsModelCategory*> getCategories();
    QHash<int, FeedsModelCategory*> getCategories(FeedsModelRootItem *root);

    bool addFeed(FeedsModelFeed *feed, int parent_id);

  protected:
    void assembleCategories(CategoryAssignment categories);
    void assembleFeeds(FeedAssignment feeds);

  private:
    FeedsModelRootItem *m_rootItem;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;

};

#endif // FEEDSMODEL_H
