// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include <QSortFilterProxyModel>

class FeedsModel;
class RootItem;

class FeedsProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit FeedsProxyModel(FeedsModel* source_model, QObject* parent = 0);
    virtual ~FeedsProxyModel();

    // Returns index list of items which "match" given value.
    // Used for finding items according to entered title text.
    QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const;

    // Maps list of indexes.
    QModelIndexList mapListToSource(const QModelIndexList& indexes) const;

    bool showUnreadOnly() const;
    void setShowUnreadOnly(bool show_unread_only);

    const RootItem* selectedItem() const;

    void setSelectedItem(const RootItem* selected_item);

  public slots:
    void invalidateReadFeedsFilter(bool set_new_value = false, bool show_unread_only = false);

  private slots:
    void invalidateFilter();

  signals:
    void expandAfterFilterIn(QModelIndex idx) const;

  private:

    // Compares two rows of data.
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
    bool filterAcceptsRowInternal(int source_row, const QModelIndex& source_parent) const;

    // Source model pointer.
    FeedsModel* m_sourceModel;
    const RootItem* m_selectedItem;
    bool m_showUnreadOnly;

    QList<QPair<int, QModelIndex>> m_hiddenIndices;
};

#endif // FEEDSPROXYMODEL_H
