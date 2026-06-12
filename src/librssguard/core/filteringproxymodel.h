// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGPROXYMODEL_H
#define FILTERINGPROXYMODEL_H

#include <QList>
#include <QMetaObject>
#include <QSortFilterProxyModel>

class FilteringProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    explicit FilteringProxyModel(QObject* parent = nullptr);

    bool hasFilteredOutItems() const;

    virtual void setSourceModel(QAbstractItemModel* source_model) override;

  signals:
    void filteredOutItemsChanged(bool has_filtered_out_items);

  protected:
    void scheduleFilteredOutItemsRefresh();

  private:
    bool containsFilteredOutItems(const QModelIndex& source_parent = {}, const QModelIndex& proxy_parent = {}) const;
    void refreshFilteredOutItemsState();
    QList<QMetaObject::Connection> connectModelSignals(const QAbstractItemModel* model);
    void clearSourceConnections();

  private:
    bool m_hasFilteredOutItems;
    bool m_refreshScheduled;
    QList<QMetaObject::Connection> m_sourceConnections;
};

#endif // FILTERINGPROXYMODEL_H
