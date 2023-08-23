// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "services/abstract/rootitem.h"

class FeedsModel;
class FeedsView;

class FeedsProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    explicit FeedsProxyModel(FeedsModel* source_model, QObject* parent = nullptr);
    virtual ~FeedsProxyModel();

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

    bool showUnreadOnly() const;
    void setShowUnreadOnly(bool show_unread_only);

    const RootItem* selectedItem() const;

    void setSelectedItem(const RootItem* selected_item);
    void setView(FeedsView* newView);

    bool sortAlphabetically() const;
    void setSortAlphabetically(bool sort_alphabetically);

  public slots:
    void invalidateReadFeedsFilter(bool set_new_value = false, bool show_unread_only = false);

  signals:
    void expandAfterFilterIn(QModelIndex source_idx) const;

    // There was some drag/drop operation, notify view about this.
    void requireItemValidationAfterDragDrop(const QModelIndex& source_index);

  protected:
    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

  private:
    virtual bool filterAcceptsRowInternal(int source_row, const QModelIndex& source_parent) const;

    // Source model pointer.
    FeedsModel* m_sourceModel;
    FeedsView* m_view;
    const RootItem* m_selectedItem;
    bool m_showUnreadOnly;
    bool m_sortAlphabetically;
    QList<QPair<int, QModelIndex>> m_hiddenIndices;
    QList<RootItem::Kind> m_priorities;
};

#endif // FEEDSPROXYMODEL_H
