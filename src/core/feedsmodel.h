// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "core/feeddownloader.h"
#include "services/abstract/rootitem.h"

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

    // Other subclassed methods.
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

    // Returns all activated service roots.
    // NOTE: Service root nodes are lying directly UNDER
    // the model root item.
    QList<ServiceRoot*> serviceRoots() const;

    // Determines if there is any account activated from given entry point.
    bool containsServiceRootFromEntryPoint(const ServiceEntryPoint *point) const;

    // Direct and the only global accessor to standard service root.
    StandardServiceRoot *standardServiceRoot() const;

    // Returns the list of feeds which should be updated
    // according to auto-update schedule.
    // Variable "auto_update_now" is true, when global timeout
    // for scheduled auto-update was met and global auto-update strategy is enabled
    // so feeds with "default" auto-update strategy should be updated.
    //
    // This method might change some properties of some feeds.
    QList<Feed*> feedsForScheduledUpdate(bool auto_update_now);

    // Returns (undeleted) messages for given feeds.
    // This is usually used for displaying whole feeds
    // in "newspaper" mode.
    QList<Message> messagesForItem(RootItem *item) const;

    // Returns ALL RECURSIVE CHILD feeds contained within single index.
    QList<Feed*> feedsForIndex(const QModelIndex &index) const;

    // Returns feed/category which lies at the specified index or
    // root item if index is invalid.
    RootItem *itemForIndex(const QModelIndex &index) const;

    // Returns source QModelIndex on which lies given item.
    // NOTE: This goes through all available indexes and
    // checks their bound items manually, there is no
    // other way to to this.
    QModelIndex indexForItem(const RootItem *item) const;

    // Determines if any feed has any new messages.
    bool hasAnyFeedNewMessages() const;

    // Access to root item.
    inline RootItem *rootItem() const {
      return m_rootItem;
    }

    // Does necessary job before quitting this component.
    void quit();

    // Adds given service root account.
    bool addServiceAccount(ServiceRoot *root, bool freshly_activated);

    // Loads feed/categories from the database.
    void loadActivatedServiceAccounts();

  public slots:
    // Checks if new parent node is different from one used by original node.
    // If it is, then it reassigns original_node to new parent.
    void reassignNodeToNewParent(RootItem *original_node, RootItem *new_parent);

    // Removes given item from the model/memory.
    void removeItem(RootItem *deleting_item);

    // Recycle bins operations.
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
    void onItemDataChanged(const QList<RootItem*> &items);

  signals:
    // Emitted if counts of messages are changed.
    void messageCountsChanged(int unread_messages, int total_messages, bool any_feed_has_unread_messages);

    // Emitted if any item requested that any view should expand it.
    void itemExpandRequested(QList<RootItem*> items, bool expand);

    // Emitted if any item requested that its expand states should be explicitly saved.
    // NOTE: Normally expand states are saved when application quits.
    void itemExpandStateSaveRequested(RootItem *subtree_root);

    // Emitted when there is a need of reloading of displayed messages.
    void reloadMessageListRequested(bool mark_selected_messages_read);

    // There was some drag/drop operation, notify view about this.
    // NOTE: View will probably expand dropped index.
    void requireItemValidationAfterDragDrop(const QModelIndex &source_index);

  private:
    RootItem *m_rootItem;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QIcon m_countsIcon;
};

#endif // FEEDSMODEL_H
