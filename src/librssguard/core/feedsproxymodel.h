// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include "services/abstract/rootitem.h"

#include <QSortFilterProxyModel>

class FeedsModel;
class FeedsView;

class FeedsProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    // Enum which describes basic filtering schemes
    // for feeds.
    enum class FeedListFilter {
      NoFiltering = 1,
      ShowUnread = 2,
      ShowEmpty = 4,
      ShowNonEmpty = 8,
      ShowWithNewArticles = 16,
      ShowWithError = 32,
      ShowSwitchedOff = 64,
      ShowQuiet = 128,
      ShowWithArticleFilters = 256
    };

    explicit FeedsProxyModel(FeedsModel* source_model, QObject* parent = nullptr);
    virtual ~FeedsProxyModel();

    void setFeedListFilter(FeedListFilter filter);

    virtual bool canDropMimeData(const QMimeData* data,
                                 Qt::DropAction action,
                                 int row,
                                 int column,
                                 const QModelIndex& parent) const;
    virtual bool dropMimeData(const QMimeData* data,
                              Qt::DropAction action,
                              int row,
                              int column,
                              const QModelIndex& parent);

    virtual void sort(int column, Qt::SortOrder order = Qt::SortOrder::AscendingOrder);

    // Returns index list of items which "match" given value.
    // Used for finding items according to entered title text.
    virtual QModelIndexList match(const QModelIndex& start,
                                  int role,
                                  const QVariant& value,
                                  int hits,
                                  Qt::MatchFlags flags) const;

    // Maps list of indexes.
    QModelIndexList mapListToSource(const QModelIndexList& indexes) const;

    const RootItem* selectedItem() const;
    void setSelectedItem(const RootItem* selected_item);

    void setView(FeedsView* newView);

    bool sortAlphabetically() const;
    void setSortAlphabetically(bool sort_alphabetically);

  signals:
    void indexNotFilteredOutAnymore(QModelIndex source_idx) const;

    // There was some drag/drop operation, notify view about this.
    void requireItemValidationAfterDragDrop(const QModelIndex& source_index);

  protected:
    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

  private:
    void initializeFilters();

    virtual bool filterAcceptsRowInternal(int source_row, const QModelIndex& source_parent) const;

    // Source model pointer.
    FeedsModel* m_sourceModel;
    FeedsView* m_view;
    const RootItem* m_selectedItem;
    bool m_sortAlphabetically;
    bool m_showNodeUnread;
    bool m_showNodeProbes;
    bool m_showNodeLabels;
    bool m_showNodeImportant;
    QList<RootItem::Kind> m_priorities;
    QList<QPair<int, QModelIndex>> m_hiddenIndices;

    FeedListFilter m_filter;

    // NOTE: The parameter type can be Category, Feed or Label only.
    QMap<FeedListFilter, std::function<bool(const RootItem*)>> m_filters;
    QList<FeedListFilter> m_filterKeys;
};

Q_DECLARE_METATYPE(FeedsProxyModel::FeedListFilter)

#endif // FEEDSPROXYMODEL_H
