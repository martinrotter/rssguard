// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QStyledItemDelegate>
#include <QTreeView>

#include "core/messagesmodel.h"
#include "core/feedsmodel.h"
#include "core/feedsselection.h"
#include "miscellaneous/settings.h"


class FeedsProxyModel;
class Feed;
class Category;
class StandardCategory;
class QTimer;

class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();

    // Fundamental accessors.
    inline FeedsProxyModel *model() {
      return m_proxyModel;
    }

    inline FeedsModel *sourceModel() {
      return m_sourceModel;
    }

    void setSortingEnabled(bool enable);
    
    // Returns list of selected/all feeds.
    // NOTE: This is recursive method which returns all descendants.
    QList<Feed*> selectedFeeds() const;
    QList<Feed*> allFeeds() const;

    // Returns pointers to selected feed/category if they are really
    // selected.
    RootItem *selectedItem() const;
    Category *selectedCategory() const;
    Feed *selectedFeed() const;

    // Saves/loads expand states of all nodes (feeds/categories) of the list to/from settings.
    void saveExpandedStates();
    void loadExpandedStates();

  public slots:
    void invalidateReadFeedsFilter(bool set_new_value = false, bool show_unread_only = false);
    void expandCollapseCurrentItem();
    void fetchMetadataForSelectedFeed();

    // Feed updating.
    void updateAllFeeds();
    void updateAllFeedsOnStartup();
    void updateSelectedFeeds();

    // Feed read/unread manipulators.
    void markSelectedFeedsReadStatus(int read);
    void markSelectedFeedsRead();
    void markSelectedFeedsUnread();
    void markAllFeedsReadStatus(int read);
    void markAllFeedsRead();

    // Newspaper accessors.
    void openSelectedFeedsInNewspaperMode();

    // Recycle bin operators.
    void emptyRecycleBin();
    void restoreRecycleBin();

    // Feed clearers.
    void setSelectedFeedsClearStatus(int clear);
    void setAllFeedsClearStatus(int clear);
    void clearSelectedFeeds();
    void clearAllFeeds();
    void clearAllReadMessages();

    // Base manipulators.
    void editSelectedItem();
    void deleteSelectedItem();

    // Is called when counts of messages are changed externally,
    // typically from message view.
    void receiveMessageCountsChange(FeedsSelection::SelectionMode mode, bool total_msg_count_changed, bool any_msg_restored);

    // Reloads counts for selected feeds.
    void updateCountsOfSelectedFeeds(bool update_total_too);

    // Reloads counts of recycle bin.
    void updateCountsOfRecycleBin(bool update_total_too);

    // Reloads counts for all feeds.
    void updateCountsOfAllFeeds(bool update_total_too);

    // Reloads counts for particular feed.
    void updateCountsOfParticularFeed(Feed *feed, bool update_total_too);

    // Notifies other components about messages
    // counts.
    inline void notifyWithCounts() {
      emit messageCountsChanged(m_sourceModel->countOfUnreadMessages(),
                                m_sourceModel->countOfAllMessages(),
                                m_sourceModel->hasAnyFeedNewMessages());
    }

    // Selects next/previous item (feed/category) in the list.
    void selectNextItem();
    void selectPreviousItem();

    // Switches visibility of the widget.
    void switchVisibility() {
      setVisible(!isVisible());
    }

  protected:
    // Initializes context menus.
    void initializeContextMenuCategories();
    void initializeContextMenuFeeds();
    void initializeContextMenuEmptySpace();

    // Sets up appearance of this widget.
    void setupAppearance();

    // Handle selections.
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    // React on "Del" key.
    void keyPressEvent(QKeyEvent *event);

    // Show custom context menu.
    void contextMenuEvent(QContextMenuEvent *event);

  private slots:
    void saveSortState(int column, Qt::SortOrder order);
    void validateItemAfterDragDrop(const QModelIndex &source_index);

  signals:
    // Emitted if user/application requested updating of some feeds.
    void feedsUpdateRequested(const QList<Feed*> feeds);

    // Emitted if counts of messages are changed.
    void messageCountsChanged(int unread_messages, int total_messages, bool any_feed_has_unread_messages);

    // Emitted if currently selected feeds needs to be reloaded.
    void feedsNeedToBeReloaded(bool mark_current_index_read);

    // Emitted if user selects new feeds.
    void feedsSelected(const FeedsSelection &selection);

    // Requests opening of given messages in newspaper mode.
    void openMessagesInNewspaperView(const QList<Message> &messages);

  private:
    QMenu *m_contextMenuCategories;
    QMenu *m_contextMenuFeeds;
    QMenu *m_contextMenuEmptySpace;

    FeedsModel *m_sourceModel;
    FeedsProxyModel *m_proxyModel;
};

#endif // FEEDSVIEW_H
