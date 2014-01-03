#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>


// TODO: http://soundguyrob.files.wordpress.com/2011/03/screen-shot-2011-03-01-at-7-45-23-pm.jpg
// přepsat počet nepřečtených zpráv podle screenshotu (tedy smazat asi sloupec "unread")
// a počet nepřečtených přes drawRow kreslit do prvního sloupce
// taky použít ten layout pro zobrazení zprávy
// NEBO pouzit delegaty (QItemDelegate nebo QStyledItemDelegate) - https://qt-project.org/forums/viewthread/24493
// + NEWSPAPER view -> v currentChanged MessagesView se
// vytáhnou všechny vybrané zprávy, dají se do QList<Message>
// a bude se emitovat signal ze se maji tyto zpravy zobrazit
// - na tohle navazat metodu v WebBrowseru, neco
// jako navigateToMessages(const QList<Message> &messages), mrknout
// do navigateToMessage

class FeedsModel;
class FeedsProxyModel;
class FeedsModelFeed;
class FeedsModelCategory;

class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();

    FeedsProxyModel *model();
    FeedsModel *sourceModel();

    // Enables or disables sorting.
    void setSortingEnabled(bool enable);
    
    // Returns list of selected/all feeds.
    QList<FeedsModelFeed*> selectedFeeds() const;
    QList<FeedsModelFeed*> allFeeds() const;

    // Return true if current index contains category/feed and
    // stores category/feed in the parameter pointer,
    // otherwise false.
    FeedsModelCategory *isCurrentIndexCategory() const;
    FeedsModelFeed *isCurrentIndexFeed() const;

  public slots:
    // Feed read/unread manipulators.
    void markSelectedFeedsReadStatus(int read);
    void markSelectedFeedsRead();
    void markSelectedFeedsUnread();

    void markAllFeedsReadStatus(int read);
    void markAllFeedsRead();

    // Feed clearers.
    void setSelectedFeedsClearStatus(int clear);
    void clearSelectedFeeds();

    // Category operators.
    void addNewCategory();
    void editSelectedItem();

    // Reloads counts for selected feeds.
    void updateCountsOfSelectedFeeds(bool update_total_too = true);

    // Reloads counts for all feeds.
    void updateCountsOfAllFeeds(bool update_total_too = true);

    void updateCountsOfParticularFeed(FeedsModelFeed *feed, bool update_total_too = true);

  protected:
    // Initializes context menus.
    void initializeContextMenuCategoriesFeeds();
    void initializeContextMenuEmptySpace();

    // Sets up appearance of this widget.
    void setupAppearance();

    // Make feeds loadable.
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);

    // Show custom context menu.
    void contextMenuEvent(QContextMenuEvent *event);

    // Don't draw branches at all.
    void drawBranches(QPainter *painter,
                      const QRect &rect,
                      const QModelIndex &index) const;

  signals:
    // Emitted if currently selected feeds needs to be reloaded.
    void feedsNeedToBeReloaded(int mark_current_index_read);

    // Emitted if user selects new feeds.
    void feedsSelected(const QList<int> &feed_ids);

  private:
    QMenu *m_contextMenuCategoriesFeeds;
    QMenu *m_contextMenuEmptySpace;

    QList<int> m_selectedFeeds;
    FeedsModel *m_sourceModel;
    FeedsProxyModel *m_proxyModel;
};

#endif // FEEDSVIEW_H
