// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGPROXYMODEL_H
#define FILTERINGPROXYMODEL_H

#include <functional>

#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QSortFilterProxyModel>

class FilteringProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    enum class FilterMatchMode {
      Any = 0,
      All = 1
    };

    explicit FilteringProxyModel(QObject* parent = nullptr);

    bool hasFilteredOutItems() const;
    FilterMatchMode filterMatchMode() const {
      return m_filterMatchMode;
    }

    virtual void setSourceModel(QAbstractItemModel* source_model) override;

  signals:
    void filteredOutItemsChanged(bool has_filtered_out_items);

  protected:
    void setFilterMatchMode(FilterMatchMode mode);
    void scheduleFilteredOutItemsRefresh();

    template <typename Filter, typename Argument>
    bool matchesSelectedFilters(Filter selected,
                                const QMap<Filter, std::function<bool(Argument)>>& predicates,
                                Argument argument) const {
      bool has_selected = false;

      for (auto it = predicates.cbegin(); it != predicates.cend(); ++it) {
        if ((static_cast<int>(selected) & static_cast<int>(it.key())) == 0) {
          continue;
        }

        has_selected = true;
        const bool matches = it.value()(argument);

        if (m_filterMatchMode == FilterMatchMode::Any && matches) {
          return true;
        }
        if (m_filterMatchMode == FilterMatchMode::All && !matches) {
          return false;
        }
      }

      return !has_selected || m_filterMatchMode == FilterMatchMode::All;
    }

    virtual bool isHierarchicalFilteringModel() const;
    virtual bool canHideRowsWithoutFilteringIndicator() const;
    virtual bool isRowHiddenWithoutFilteringIndicator(const QModelIndex& source_index) const;

  private:
    bool containsFilteredOutItems(const QModelIndex& source_parent = {}, const QModelIndex& proxy_parent = {}) const;
    void refreshFilteredOutItemsState();
    QList<QMetaObject::Connection> connectModelSignals(const QAbstractItemModel* model);
    void clearSourceConnections();

  private:
    FilterMatchMode m_filterMatchMode;
    bool m_hasFilteredOutItems;
    bool m_refreshScheduled;
    QList<QMetaObject::Connection> m_sourceConnections;
};

Q_DECLARE_METATYPE(FilteringProxyModel::FilterMatchMode)

#endif // FILTERINGPROXYMODEL_H
