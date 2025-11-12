// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include "core/feedsmodel.h"
#include "gui/reusable/basetreeview.h"
#include "gui/toolbars/feedstoolbar.h"

#include <QStyledItemDelegate>
#include <QTimer>

class FeedsProxyModel;
class Feed;
class Category;
class StyledItemDelegate;

class RSSGUARD_DLLSPEC FeedsView : public BaseTreeView {
    Q_OBJECT

  public:
    explicit FeedsView(QWidget* parent = nullptr);
    virtual ~FeedsView();

    FeedsProxyModel* model() const;
    FeedsModel* sourceModel() const;

    void reloadFontSettings();

    void setSortingEnabled(bool enable);

    // Returns list of selected/all feeds.
    // NOTE: This is recursive method which returns all descendants.
    QList<Feed*> selectedFeeds(bool recursive) const;

    // Returns selected item. If multiple items are selected, returns
    // the one of them which is also "current". If none of them is
    // "current", returns firs item of selected ones.
    RootItem* selectedItem() const;
    QList<RootItem*> selectedItems() const;

    // Saves/loads expand states of all nodes (feeds/categories) of the list to/from settings.
    void saveAllExpandStates();
    void loadAllExpandStates();

    QByteArray saveHeaderState() const;
    void restoreHeaderState(const QByteArray& dta);

  public slots:
    void revealItem(RootItem* item);
    void copyUrlOfSelectedFeeds() const;
    void sortByColumn(int column, Qt::SortOrder order);

    void addFeedIntoSelectedAccount();
    void addCategoryIntoSelectedAccount();
    void expandCollapseCurrentItem(bool recursive);

    // Feed updating.
    void updateSelectedItems();

    // Feed read/unread manipulators.
    void markSelectedItemRead();
    void markSelectedItemUnread();
    void markAllItemsRead();

    // Feed clearers.
    void clearSelectedItems();
    void clearAllItems();
    void purgeSelectedFeeds();

    // Base manipulators.
    void editItems(const QList<RootItem*>& items);
    void editSelectedItems();
    void editChildFeeds();
    void editRecursiveFeeds();
    void deleteSelectedItem();

    // Sort order manipulations.
    void moveSelectedItemUp();
    void moveSelectedItemTop();
    void moveSelectedItemBottom();
    void moveSelectedItemDown();
    void rearrangeCategoriesOfSelectedItem();
    void rearrangeFeedsOfSelectedItem();

    // Selects next/previous item (feed/category) in the list.
    void selectNextItem();
    void selectPreviousItem();
    void selectNextUnreadItem();

    // Switches visibility of the widget.
    void switchVisibility();

    void changeFilter(FeedsProxyModel::FeedListFilter filter);
    void filterItems(SearchLineEdit::SearchMode mode,
                     Qt::CaseSensitivity sensitivity,
                     int custom_criteria,
                     const QString& phrase);
    void toggleFeedSortingMode(bool sort_alphabetically);

  signals:
    void itemSelected(RootItem* item);
    void requestViewNextUnreadMessage();

  protected:
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const;
    virtual void drawRow(QPainter* painter, const QStyleOptionViewItem& options, const QModelIndex& index) const;

  private slots:
    void adjustColumns();
    void onIndexExpanded(const QModelIndex& idx);
    void onIndexCollapsed(const QModelIndex& idx);

    void reloadDelayedExpansions();
    void reloadItemExpandState(const QModelIndex& source_idx);
    void markSelectedItemReadStatus(RootItem::ReadStatus read);
    void markAllItemsReadStatus(RootItem::ReadStatus read);

    void saveSortState(int column, Qt::SortOrder order);
    void validateItemAfterDragDrop(const QModelIndex& source_index);
    void onItemExpandRequested(const QList<RootItem*>& items, bool exp);
    void onItemExpandStateSaveRequested(RootItem* item);

  private:
    QModelIndex nextPreviousUnreadItem(const QModelIndex& default_row);
    QModelIndex nextUnreadItem(const QModelIndex& default_row);

    // Initializes context menus.
    QMenu* initializeContextMenuBin(RootItem* clicked_item);
    QMenu* initializeContextMenuService(RootItem* clicked_item);
    QMenu* initializeContextMenuCategories(RootItem* clicked_item);
    QMenu* initializeContextMenuFeeds(RootItem* clicked_item);
    QMenu* initializeContextMenuImportant(RootItem* clicked_item);
    QMenu* initializeContextMenuOtherItem(RootItem* clicked_item);
    QMenu* initializeContextMenuLabel(RootItem* clicked_item);
    QMenu* initializeContextMenuProbe(RootItem* clicked_item);

    void setupAppearance();
    void saveExpandStates(RootItem* item);

    QMenu* m_contextMenuService;
    QMenu* m_contextMenuBin;
    QMenu* m_contextMenuCategories;
    QMenu* m_contextMenuFeeds;
    QMenu* m_contextMenuImportant;
    QMenu* m_contextMenuOtherItems;
    QMenu* m_contextMenuLabel;
    QMenu* m_contextMenuProbe;
    FeedsModel* m_sourceModel;
    FeedsProxyModel* m_proxyModel;
    bool m_dontSaveExpandState;
    QList<QPair<QModelIndex, bool>> m_delayedItemExpansions;
    QTimer m_expansionDelayer;
    StyledItemDelegate* m_delegate;
    bool m_columnsAdjusted;
};

inline FeedsProxyModel* FeedsView::model() const {
  return m_proxyModel;
}

inline FeedsModel* FeedsView::sourceModel() const {
  return m_sourceModel;
}

#endif // FEEDSVIEW_H
