#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>

#include "core/messagesmodel.h"
#include "core/feedsmodel.h"


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

class FeedsProxyModel;
class FeedsModelFeed;
class FeedsModelCategory;

class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();

    inline FeedsProxyModel *model() {
      return m_proxyModel;
    }

    inline FeedsModel *sourceModel() {
      return m_sourceModel;
    }

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

    // Newspaper accessors.
    void openSelectedFeedsInNewspaperMode();

    // Feed clearers.
    void setSelectedFeedsClearStatus(int clear);
    void clearSelectedFeeds();

    // Category operators.
    void addNewCategory();
    void editSelectedItem();
    void deleteSelectedItem();

    // Reloads counts for selected feeds.
    void updateCountsOfSelectedFeeds(bool update_total_too = true);

    // Reloads counts for all feeds.
    void updateCountsOfAllFeeds(bool update_total_too = true);

    // Reloads counts for particular feed.
    void updateCountsOfParticularFeed(FeedsModelFeed *feed, bool update_total_too = true);

    // Notifies other components about messages
    // counts.
    inline void notifyWithCounts() {
      emit feedCountsChanged(m_sourceModel->countOfUnreadMessages(),
                             m_sourceModel->countOfAllMessages());
    }

  protected:
    // Initializes context menus.
    void initializeContextMenuCategoriesFeeds();
    void initializeContextMenuEmptySpace();

    // Sets up appearance of this widget.
    void setupAppearance();

    // Handle selections.
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);

    // Show custom context menu.
    void contextMenuEvent(QContextMenuEvent *event);

  signals:
    // Emitted if counts of messages are changed.
    void feedCountsChanged(int unread_messages, int total_messages);

    // Emitted if currently selected feeds needs to be reloaded.
    void feedsNeedToBeReloaded(int mark_current_index_read);

    // Emitted if user selects new feeds.
    void feedsSelected(const QList<int> &feed_ids);

    // Requests opening of given messages in newspaper mode.
    void openMessagesInNewspaperView(const QList<Message> &messages);

  private:
    QMenu *m_contextMenuCategoriesFeeds;
    QMenu *m_contextMenuEmptySpace;

    QList<int> m_selectedFeeds;
    FeedsModel *m_sourceModel;
    FeedsProxyModel *m_proxyModel;
};

#endif // FEEDSVIEW_H
