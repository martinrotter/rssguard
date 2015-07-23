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
#include "core/feedsmodelrootitem.h"

#include <QIcon>


class FeedsModelCategory;
class FeedsModelFeed;
class FeedsModelRecycleBin;
class FeedsImportExportModel;

typedef QList<QPair<int, FeedsModelCategory*> > CategoryAssignment;
typedef QPair<int, FeedsModelCategory*> CategoryAssignmentItem;

typedef QList<QPair<int, FeedsModelFeed*> > FeedAssignment;
typedef QPair<int, FeedsModelFeed*> FeedAssignmentItem;

class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

    friend class FeedsModelFeed;
    friend class FeedsModelCategory;

  public:
    // Constructors and destructors.
    explicit FeedsModel(QObject *parent = 0);
    virtual ~FeedsModel();

    // Model implementation.
    inline QVariant data(const QModelIndex &index, int role) const {
      // Return data according to item.
      return itemForIndex(index)->data(index.column(), role);
    }

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

    // Removes item with given index.
    bool removeItem(const QModelIndex &index);

    // Standard category manipulators.
    bool addCategory(FeedsModelCategory *category, FeedsModelRootItem *parent);
    bool editCategory(FeedsModelCategory *original_category, FeedsModelCategory *new_category_data);

    // Standard feed manipulators.
    bool addFeed(FeedsModelFeed *feed, FeedsModelRootItem *parent);

    // New feed is just temporary feed, it is not added to the model.
    // It is used to fetch its data to the original feed
    // and the original feed is moved if needed.
    bool editFeed(FeedsModelFeed *original_feed, FeedsModelFeed *new_feed_data);

    // Returns the list of updates which should be updated
    // according to auto-update schedule.
    // Variable "auto_update_now" is true, when global timeout
    // for scheduled auto-update was met so feeds with "default"
    // auto-update strategy should be updated.
    QList<FeedsModelFeed*> feedsForScheduledUpdate(bool auto_update_now);

    // Returns (undeleted) messages for given feeds.
    // This is usually used for displaying whole feeds
    // in "newspaper" mode.
    QList<Message> messagesForFeeds(const QList<FeedsModelFeed*> &feeds);

    // Returns all categories, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, FeedsModelCategory*> allCategories();

    // Returns categories from the subtree with given root node, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int, FeedsModelCategory*> categoriesForItem(FeedsModelRootItem *root);

    // Returns list of all feeds contained in the model.
    QList<FeedsModelFeed*> allFeeds();

    // Get list of feeds from tree with particular item
    // as root. If root itself is a feed, then it is returned.
    QList<FeedsModelFeed*> feedsForItem(FeedsModelRootItem *root);

    // Returns list of ALL CHILD feeds which belong to given parent indexes.
    QList<FeedsModelFeed*> feedsForIndexes(const QModelIndexList &indexes);

    // Returns ALL CHILD feeds contained within single index.
    QList<FeedsModelFeed*> feedsForIndex(const QModelIndex &index);

    // Returns pointer to feed if it lies on given index
    // or NULL if no feed lies on given index.
    FeedsModelFeed *feedForIndex(const QModelIndex &index);

    // Returns pointer to category if it lies on given index
    // or NULL if no category lies on given index.
    FeedsModelCategory *categoryForIndex(const QModelIndex &index) const;

    // Returns pointer to recycle bin if lies on given index
    // or NULL if no recycle bin lies on given index.
    FeedsModelRecycleBin *recycleBinForIndex(const QModelIndex &index) const;

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    FeedsModelRootItem *itemForIndex(const QModelIndex &index) const;

    // Returns source QModelIndex on which lies given item.
    QModelIndex indexForItem(FeedsModelRootItem *item) const;

    bool hasAnyFeedNewMessages();

    // Access to root item.
    inline FeedsModelRootItem *rootItem() const {
      return m_rootItem;
    }

    // Takes structure residing under given root item and adds feeds/categories from
    // it to active structure.
    bool mergeModel(FeedsImportExportModel *model, QString &output_message);

    // Access to recycle bin.
    FeedsModelRecycleBin *recycleBin() const;

  public slots:
    // Feeds operations.
    bool markFeedsRead(const QList<FeedsModelFeed*> &feeds, int read);
    bool markFeedsDeleted(const QList<FeedsModelFeed*> &feeds, int deleted, bool read_only);

    // Signals that properties (probably counts)
    // of ALL items have changed.
    void reloadWholeLayout();

    // Signals that SOME data of this model need
    // to be reloaded by ALL attached views.
    // NOTE: This reloads all parent valid indexes too.
    void reloadChangedLayout(QModelIndexList list);

  protected:
    // Returns converted ids of given feeds
    // which are suitable as IN clause for SQL queries.
    QStringList textualFeedIds(const QList<FeedsModelFeed*> &feeds);

    // Loads feed/categories from the database.
    void loadFromDatabase();

    // Takes lists of feeds/categories and assembles
    // them into the tree structure.
    void assembleCategories(CategoryAssignment categories);
    void assembleFeeds(FeedAssignment feeds);

  signals:
    void requireItemValidationAfterDragDrop(const QModelIndex &source_index);

  private:
    FeedsModelRootItem *m_rootItem;
    FeedsModelRecycleBin *m_recycleBin;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;
};

#endif // FEEDSMODEL_H
