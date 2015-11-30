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

#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QAbstractItemModel>

#include "core/message.h"
#include "core/rootitem.h"
#include "core/feeddownloader.h"

class DatabaseCleaner;
class Category;
class Feed;
class ServiceRoot;
class ServiceEntryPoint;
class StandardServiceRoot;
class QTimer;

class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsModel(QObject *parent = 0);
    virtual ~FeedsModel();

    DatabaseCleaner *databaseCleaner();

    // Model implementation.
    inline QVariant data(const QModelIndex &index, int role) const {
      // Return data according to item.
      return itemForIndex(index)->data(index.column(), role);
    }

    // Drag & drop.
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;

    // Returns counts of ALL/UNREAD (non-deleted) messages for the model.
    inline int countOfAllMessages() const {
      return m_rootItem->countOfAllMessages();
    }

    inline int countOfUnreadMessages() const {
      return m_rootItem->countOfUnreadMessages();
    }

    void reloadCountsOfWholeModel();

    // Removes item with given index.
    // NOTE: Also deletes item from memory.
    void removeItem(const QModelIndex &index);
    void removeItem(RootItem *deleting_item);

    // Checks if new parent node is different from one used by original node.
    // If it is, then it reassigns original_node to new parent.
    void reassignNodeToNewParent(RootItem *original_node, RootItem *new_parent);

    // Returns all activated service roots.
    // NOTE: Service root nodes are lying directly UNDER
    // the model root item.
    QList<ServiceRoot*> serviceRoots();

    // Determines if there is any account activated from given entry point.
    bool containsServiceRootFromEntryPoint(ServiceEntryPoint *point);

    // Direct and the only global accessor to standard service root.
    // NOTE: Standard service root is always activated.
    StandardServiceRoot *standardServiceRoot();

    // Returns the list of feeds which should be updated
    // according to auto-update schedule.
    // Variable "auto_update_now" is true, when global timeout
    // for scheduled auto-update was met and global auto-update strategy is enabled
    // so feeds with "default" auto-update strategy should be updated.
    QList<Feed*> feedsForScheduledUpdate(bool auto_update_now);

    // Returns (undeleted) messages for given feeds.
    // This is usually used for displaying whole feeds
    // in "newspaper" mode.
    QList<Message> messagesForFeeds(const QList<Feed*> &feeds);

    // Returns list of all categories contained in the model.
    QList<Category*> allCategories();

    // Returns list of all feeds contained in the model.
    QList<Feed*> allFeeds();

    // Returns ALL RECURSIVE CHILD feeds contained within single index.
    QList<Feed*> feedsForIndex(const QModelIndex &index);

    // Returns pointer to feed if it lies on given index
    // or NULL if no feed lies on given index.
    Feed *feedForIndex(const QModelIndex &index);

    // Returns pointer to category if it lies on given index
    // or NULL if no category lies on given index.
    Category *categoryForIndex(const QModelIndex &index) const;

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    RootItem *itemForIndex(const QModelIndex &index) const;

    // Returns source QModelIndex on which lies given item.
    // NOTE: This goes through all available indexes and
    // checks their bound items manually, there is no
    // other way to to this.
    QModelIndex indexForItem(RootItem *item) const;

    // Determines if any feed has any new messages.
    bool hasAnyFeedNewMessages();

    // Access to root item.
    inline RootItem *rootItem() const {
      return m_rootItem;
    }

    // Resets global auto-update intervals according to settings
    // and starts/stop the timer as needed.
    void updateAutoUpdateStatus();

    // Does necessary job before quitting this component.
    void quit();

    // Schedules given feeds for update.
    void updateFeeds(const QList<Feed*> &feeds);

    // Schedules all feeds from all accounts for update.
    void updateAllFeeds();

    // Adds given service root account.
    bool addServiceAccount(ServiceRoot *root);

  public slots:
    bool restoreAllBins();
    bool emptyAllBins();

    // Feeds operations.
    bool markItemRead(RootItem *item, RootItem::ReadStatus read);
    bool markItemCleared(RootItem *item, bool clean_read_only);

    // Signals that properties (probably counts)
    // of ALL items have changed.
    void reloadWholeLayout();

    // Signals that SOME data of this model need
    // to be reloaded by ALL attached views.
    // NOTE: This reloads all parent valid indexes too.
    void reloadChangedLayout(QModelIndexList list);

    // Invalidates data under index for the item.
    void reloadChangedItem(RootItem *item);

    // Notifies other components about messages
    // counts.
    void notifyWithCounts();

  private slots:
    void onItemDataChanged(QList<RootItem*> items);

    // Is executed when next auto-update round could be done.
    void executeNextAutoUpdate();

    // Reacts on feed updates.
    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(Feed *feed, int current, int total);
    void onFeedUpdatesFinished(FeedDownloadResults results);

  signals:
    // Update of feeds is finished.
    void feedsUpdateFinished();

    // Counts of unread messages are changed in some feeds,
    // notify view about this shit.
    void readFeedsFilterInvalidationRequested();

    // Emitted when model requests update of some feeds.
    void feedsUpdateRequested(const QList<Feed*> feeds);

    // Emitted if counts of messages are changed.
    void messageCountsChanged(int unread_messages, int total_messages, bool any_feed_has_unread_messages);

    // Emitted when there is a need of reloading of displayed messages.
    void reloadMessageListRequested(bool mark_selected_messages_read);

    // There was some drag/drop operation, notify view about this.
    // NOTE: View will probably expand dropped index.
    void requireItemValidationAfterDragDrop(const QModelIndex &source_index);

  private:
    // Returns converted ids of given feeds
    // which are suitable as IN clause for SQL queries.
    QStringList textualFeedIds(const QList<Feed*> &feeds);

    // Loads feed/categories from the database.
    void loadActivatedServiceAccounts();

    RootItem *m_rootItem;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;

    // Auto-update stuff.
    QTimer *m_autoUpdateTimer;
    bool m_globalAutoUpdateEnabled;
    int m_globalAutoUpdateInitialInterval;
    int m_globalAutoUpdateRemainingInterval;

    QThread *m_feedDownloaderThread;
    FeedDownloader *m_feedDownloader;

    QThread *m_dbCleanerThread;
    DatabaseCleaner *m_dbCleaner;
};

#endif // FEEDSMODEL_H
