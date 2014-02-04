#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>

#include "core/messagesmodel.h"
#include "core/feedsmodel.h"
#include "core/settings.h"


class FeedsProxyModel;
class FeedsModelFeed;
class FeedsModelCategory;
class QTimer;

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

    // Does necessary job before quitting this component.
    void quit();

    // Resets global auto-update intervals according to settings
    // and starts/stop the timer as needed.
    void updateAutoUpdateStatus();

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
    // Feed updating.
    void updateAllFeeds();
    void updateSelectedFeeds();

    // Is executed when next auto-update round could be done.
    void executeNextAutoUpdate();

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
    void clearAllReadMessages();

    // Base manipulators.
    void editSelectedItem();
    void deleteSelectedItem();

    // Standard category manipulators.
    void addNewStandardCategory();
    void editStandardCategory(FeedsModelStandardCategory *category);

    // Standard feed manipulators.
    void addNewStandardFeed();
    void editStandardFeed(FeedsModelStandardFeed *feed);

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

    void keyPressEvent(QKeyEvent *event);

    // Show custom context menu.
    void contextMenuEvent(QContextMenuEvent *event);

  signals:
    // Emitted if user/application requested updating of some feeds.
    void feedsUpdateRequested(const QList<FeedsModelFeed*> feeds);

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

    // Auto-update stuff.
    QTimer *m_autoUpdateTimer;
    bool m_globalAutoUpdateEnabled;
    int m_globalAutoUpdateInitialInterval;
    int m_globalAutoUpdateRemainingInterval;
};

#endif // FEEDSVIEW_H
