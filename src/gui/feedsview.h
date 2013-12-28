#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>


class FeedsModel;
class FeedsProxyModel;
class FeedsModelFeed;

class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();

    // Enables or disables sorting.
    void setSortingEnabled(bool enable);
    
    // Returns list of selected/all feeds.
    QList<FeedsModelFeed*> selectedFeeds() const;
    QList<FeedsModelFeed*> allFeeds() const;

  public slots:
    // Feed read/unread manipulators.
    void markSelectedFeedsReadStatus(int read);
    void markSelectedFeedsRead();
    void markSelectedFeedsUnread();

    // Feed clearers.
    void setSelectedFeedsClearStatus(int clear);
    void clearSelectedFeeds();

    // Reloads counts for selected feeds.
    void updateCountsOfSelectedFeeds(bool update_total_too = true);

    // Reloads counts for all feeds.
    void updateCountsOfAllFeeds(bool update_total_too = true);

  protected:
    void initializeContextMenu();

    // Sets up appearance of this widget.
    void setupAppearance();

    // Make feeds loadable.
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);

    // Show custom context menu.
    void contextMenuEvent(QContextMenuEvent *event);

  signals:
    // Emitted if currently selected feeds needs to be reloaded.
    void feedsNeedToBeReloaded(int mark_current_index_read);

    // Emitted if user selects new feeds.
    void feedsSelected(const QList<int> &feed_ids);

  private:
    QMenu *m_contextMenu;

    QList<int> m_selectedFeeds;
    FeedsModel *m_sourceModel;
    FeedsProxyModel *m_proxyModel;
};

#endif // FEEDSVIEW_H
