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

#ifndef SERVICEROOT_H
#define SERVICEROOT_H

#include "services/abstract/rootitem.h"

#include "core/message.h"

#include <QPair>


class FeedsModel;
class RecycleBin;
class QAction;
class QSqlTableModel;

// Car here represents ID of the item.
typedef QList<QPair<int,RootItem*> > Assignment;
typedef QPair<int,RootItem*> AssignmentItem;

// THIS IS the root node of the service.
// NOTE: The root usually contains some core functionality of the
// service like service account username/password etc.
class ServiceRoot : public RootItem {
    Q_OBJECT

  public:
    explicit ServiceRoot(RootItem *parent = NULL);
    virtual ~ServiceRoot();

    /////////////////////////////////////////
    // /* Members to override.
    /////////////////////////////////////////

    bool deleteViaGui();
    bool markAsReadUnread(ReadStatus status);

    virtual bool supportsFeedAdding() const = 0;
    virtual bool supportsCategoryAdding() const = 0;

    // Returns list of specific actions for "Add new item" main window menu.
    // So typical list of returned actions could look like:
    //  a) Add new feed
    //  b) Add new category
    //  c) ...
    // NOTE: Caller does NOT take ownership of created menu!
    virtual QList<QAction*> addItemMenu() = 0;

    // Returns list of specific actions to be shown in main window menu
    // bar in sections "Services -> 'this service'".
    // NOTE: Caller does NOT take ownership of created menu!
    virtual QList<QAction*> serviceMenu() = 0;

    // Access to recycle bin of this account if there is any.
    virtual RecycleBin *recycleBin() = 0;

    QList<Message> undeletedMessages() const;

    // Start/stop services.
    // Start method is called when feed model gets initialized OR after user adds new service.
    // Account should synchronously initialize its children (load them from DB is recommended
    // here).
    //
    // Stop method is called just before application exits OR when
    // user explicitly deletes existing service instance.
    virtual void start(bool freshly_activated) = 0;
    virtual void stop() = 0;

    // Returns the UNIQUE code of the given service.
    // NOTE: Keep in sync with ServiceEntryRoot::code().
    virtual QString code() = 0;

    // This method should prepare messages for given "item" (download them maybe?)
    // into predefined "Messages" table
    // and then use method QSqlTableModel::setFilter(....).
    // NOTE: It would be more preferable if all messages are downloaded
    // right when feeds are updated.
    virtual bool loadMessagesForItem(RootItem *item, QSqlTableModel *model) = 0;

    // Called BEFORE this read status update (triggered by user in message list) is stored in DB,
    // when false is returned, change is aborted.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.
    //
    // "read" is status which is ABOUT TO BE SET.
    virtual bool onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, ReadStatus read) = 0;

    // Called AFTER this read status update (triggered by user in message list) is stored in DB,
    // when false is returned, change is aborted.
    // Here service root should inform (via signals)
    // which items are actually changed.
    //
    // "read" is status which is ABOUT TO BE SET.
    virtual bool onAfterSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, ReadStatus read) = 0;

    // Called BEFORE this importance switch update is stored in DB,
    // when false is returned, change is aborted.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.
    //
    // "changes" - list of pairs - <message (integer id), new status>
    virtual bool onBeforeSwitchMessageImportance(RootItem *selected_item, const QList<QPair<Message,RootItem::Importance> > &changes) = 0;

    // Called AFTER this importance switch update is stored in DB,
    // when false is returned, change is aborted.
    // Here service root should inform (via signals)
    // which items are actually changed.
    //
    // "changes" - list of pairs - <message (integer id), new status>
    virtual bool onAfterSwitchMessageImportance(RootItem *selected_item, const QList<QPair<Message,RootItem::Importance> > &changes) = 0;

    // Called BEFORE the list of messages is about to be deleted
    // by the user from message list.
    virtual bool onBeforeMessagesDelete(RootItem *selected_item, const QList<Message> &messages) = 0;

    // Called AFTER the list of messages was deleted
    // by the user from message list.
    virtual bool onAfterMessagesDelete(RootItem *selected_item, const QList<Message> &messages) = 0;

    // Called BEFORE the list of messages is about to be restored from recycle bin
    // by the user from message list.
    // Selected item is naturally recycle bin.
    virtual bool onBeforeMessagesRestoredFromBin(RootItem *selected_item, const QList<Message> &messages) = 0;

    // Called AFTER the list of messages was restored from recycle bin
    // by the user from message list.
    // Selected item is naturally recycle bin.
    virtual bool onAfterMessagesRestoredFromBin(RootItem *selected_item, const QList<Message> &messages) = 0;

    /////////////////////////////////////////
    // Members to override. */
    /////////////////////////////////////////

    // Obvious methods to wrap signals.
    void itemChanged(const QList<RootItem*> &items);
    void requestReloadMessageList(bool mark_selected_messages_read);
    void requestFeedReadFilterReload();
    void requestItemExpand(const QList<RootItem*> &items, bool expand);
    void requestItemReassignment(RootItem *item, RootItem *new_parent);
    void requestItemRemoval(RootItem *item);

    // Account ID corresponds with DB attribute Accounts (id).
    int accountId() const;
    void setAccountId(int account_id);

  public slots:
    virtual void addNewFeed(const QString &url = QString()) = 0;
    virtual void addNewCategory() = 0;

  protected:
    // Takes lists of feeds/categories and assembles them into the tree structure.
    void assembleCategories(Assignment categories);
    void assembleFeeds(Assignment feeds);

  signals:
    // Emitted if data in any item belonging to this root are changed.
    void dataChanged(QList<RootItem*> items);
    void readFeedsFilterInvalidationRequested();
    void reloadMessageListRequested(bool mark_selected_messages_read);
    void itemExpandRequested(QList<RootItem*> items, bool expand);

    void itemReassignmentRequested(RootItem *item, RootItem *new_parent);
    void itemRemovalRequested(RootItem *item);

  private:
    int m_accountId;
};

#endif // SERVICEROOT_H
