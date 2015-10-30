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

#include "core/messagesmodel.h"
#include "core/rootitem.h"

#include <QIcon>


class StandardCategory;
class Feed;
class StandardRecycleBin;
class FeedsImportExportModel;
class QTimer;

class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsModel(QObject *parent = 0);
    virtual ~FeedsModel();

    // Model implementation.
    inline QVariant data(const QModelIndex &index, int role) const {
      // Return data according to item.
      return itemForIndex(index)->data(index.column(), role);
    }

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

    // Removes item with given index.
    bool removeItem(const QModelIndex &index);

    // Standard category manipulators.
    bool addCategory(StandardCategory *category, RootItem *parent);

    // Standard feed manipulators.
    bool addFeed(StandardFeed *feed, RootItem *parent);

    // Checks if new parent node is different from one used by original node.
    // If it is, then it reassigns original_node to new parent.
    void reassignNodeToNewParent(RootItem *original_node, RootItem *new_parent);

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

    // Returns all categories, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, StandardCategory*> allCategories();

    // Returns categories from the subtree with given root node, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, StandardCategory*> categoriesForItem(RootItem *root);

    // Returns list of all feeds contained in the model.
    QList<Feed*> allFeeds();

    // Get list of feeds from tree with particular item
    // as root. If root itself is a feed, then it is returned.
    QList<Feed*> feedsForItem(RootItem *root);

    // Returns list of ALL CHILD feeds which belong to given parent indexes.
    QList<Feed*> feedsForIndexes(const QModelIndexList &indexes);

    // Returns ALL CHILD feeds contained within single index.
    QList<Feed*> feedsForIndex(const QModelIndex &index);

    // Returns pointer to feed if it lies on given index
    // or NULL if no feed lies on given index.
    Feed *feedForIndex(const QModelIndex &index);

    // Returns pointer to category if it lies on given index
    // or NULL if no category lies on given index.
    StandardCategory *categoryForIndex(const QModelIndex &index) const;

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    RootItem *itemForIndex(const QModelIndex &index) const;

    // Returns source QModelIndex on which lies given item.
    QModelIndex indexForItem(RootItem *item) const;

    // Determines if any feed has any new messages.
    bool hasAnyFeedNewMessages();

    // Access to root item.
    inline RootItem *rootItem() const {
      return m_rootItem;
    }

    // Takes structure residing under given root item and adds feeds/categories from
    // it to active structure.
    bool mergeModel(FeedsImportExportModel *model, QString &output_message);

    // Resets global auto-update intervals according to settings
    // and starts/stop the timer as needed.
    void updateAutoUpdateStatus();

    // Does necessary job before quitting this component.
    void quit();

  public slots:
    // Feeds operations.
    bool markFeedsRead(const QList<Feed*> &feeds, int read);
    bool markFeedsDeleted(const QList<Feed*> &feeds, int deleted, bool read_only);

    // Signals that properties (probably counts)
    // of ALL items have changed.
    void reloadWholeLayout();

    // Signals that SOME data of this model need
    // to be reloaded by ALL attached views.
    // NOTE: This reloads all parent valid indexes too.
    void reloadChangedLayout(QModelIndexList list);

  private slots:
    // Is executed when next auto-update round could be done.
    void executeNextAutoUpdate();

  protected:
    // Returns converted ids of given feeds
    // which are suitable as IN clause for SQL queries.
    QStringList textualFeedIds(const QList<Feed*> &feeds);

    // Loads feed/categories from the database.
    void loadActivatedServiceAccounts();

  signals:
    // Emitted when model requests update of some feeds.
    void feedsUpdateRequested(const QList<Feed*> feeds);

  private:
    RootItem *m_rootItem;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;

    // Auto-update stuff.
    QTimer *m_autoUpdateTimer;
    bool m_globalAutoUpdateEnabled;
    int m_globalAutoUpdateInitialInterval;
    int m_globalAutoUpdateRemainingInterval;
};

#endif // FEEDSMODEL_H
