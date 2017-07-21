// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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
class MessagesModel;

// Car here represents ID of the item.
typedef QList<QPair<int, RootItem*>> Assignment;
typedef QPair<int, RootItem*> AssignmentItem;
typedef QPair<Message, RootItem::Importance> ImportanceChange;

// THIS IS the root node of the service.
// NOTE: The root usually contains some core functionality of the
// service like service account username/password etc.
class ServiceRoot : public RootItem {
		Q_OBJECT

	public:
		explicit ServiceRoot(RootItem* parent = nullptr);
		virtual ~ServiceRoot();

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
		virtual QList<QAction*> addItemMenu();

		// Returns actions to display as context menu.
		QList<QAction*> contextMenu();

		// Returns list of specific actions to be shown in main window menu
		// bar in sections "Services -> 'this service'".
		// NOTE: Caller does NOT take ownership of created menu!
		virtual QList<QAction*> serviceMenu();

		// Access to recycle bin of this account if there is any.
		virtual RecycleBin* recycleBin() const = 0;

		void updateCounts(bool including_total_count);

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

		virtual void saveAllCachedData();

		// Account ID corresponds with DB attribute Accounts (id).
		int accountId() const;
		void setAccountId(int account_id);

		// Returns the UNIQUE code of the given service.
		// NOTE: Keep in sync with ServiceEntryRoot::code().
		virtual QString code() const = 0;

		// Removes all/read only messages from given underlying feeds.
		bool cleanFeeds(QList<Feed*> items, bool clean_read_only);

		// This method should prepare messages for given "item" (download them maybe?)
		// into predefined "Messages" table
		// and then use method QSqlTableModel::setFilter(....).
		// NOTE: It would be more preferable if all messages are downloaded
		// right when feeds are updated.
		virtual bool loadMessagesForItem(RootItem* item, MessagesModel* model);

		// Called BEFORE this read status update (triggered by user in message list) is stored in DB,
		// when false is returned, change is aborted.
		// This is the place to make some other changes like updating
		// some ONLINE service or something.
		//
		// "read" is status which is ABOUT TO BE SET.
		virtual bool onBeforeSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);

		// Called AFTER this read status update (triggered by user in message list) is stored in DB,
		// when false is returned, change is aborted.
		// Here service root should inform (via signals)
		// which items are actually changed.
		//
		// "read" is status which is ABOUT TO BE SET.
		virtual bool onAfterSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);

		// Called BEFORE this importance switch update is stored in DB,
		// when false is returned, change is aborted.
		// This is the place to make some other changes like updating
		// some ONLINE service or something.
		//
		// "changes" - list of pairs - <message (integer id), new status>
		virtual bool onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

		// Called AFTER this importance switch update is stored in DB,
		// when false is returned, change is aborted.
		// Here service root should inform (via signals)
		// which items are actually changed.
		//
		// "changes" - list of pairs - <message (integer id), new status>
		virtual bool onAfterSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

		// Called BEFORE the list of messages is about to be deleted
		// by the user from message list.
		virtual bool onBeforeMessagesDelete(RootItem* selected_item, const QList<Message>& messages);

		// Called AFTER the list of messages was deleted
		// by the user from message list.
		virtual bool onAfterMessagesDelete(RootItem* selected_item, const QList<Message>& messages);

		// Called BEFORE the list of messages is about to be restored from recycle bin
		// by the user from message list.
		// Selected item is naturally recycle bin.
		virtual bool onBeforeMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

		// Called AFTER the list of messages was restored from recycle bin
		// by the user from message list.
		// Selected item is naturally recycle bin.
		virtual bool onAfterMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages);

		void completelyRemoveAllData();
		QStringList customIDSOfMessagesForItem(RootItem* item);
		bool markFeedsReadUnread(QList<Feed*> items, ReadStatus read);

		// Obvious methods to wrap signals.
		void itemChanged(const QList<RootItem*>& items);
		void requestReloadMessageList(bool mark_selected_messages_read);
		void requestItemExpand(const QList<RootItem*>& items, bool expand);
		void requestItemExpandStateSave(RootItem* subtree_root);
		void requestItemReassignment(RootItem* item, RootItem* new_parent);
		void requestItemRemoval(RootItem* item);

	public slots:
		virtual void addNewFeed(const QString& url = QString()) = 0;
		virtual void addNewCategory() = 0;
		virtual void syncIn();

	protected:
		// This method should obtain new tree of feed/messages/etc to perform
		// sync in.
		virtual RootItem* obtainNewTreeForSyncIn() const;

		// Removes all messages/categories/feeds which are
		// associated with this account.
		void removeOldFeedTree(bool including_messages);
		void storeNewFeedTree(RootItem* root);
		void cleanAllItems();

		// Removes messages which do not belong to any
		// existing feed.
		//
		// NOTE: This situation may happen if user deletes some feed
		// from another machine and then performs sync-in on this machine.
		void removeLeftOverMessages();

		QStringList textualFeedIds(const QList<Feed*>& feeds) const;
		QStringList customIDsOfMessages(const QList<ImportanceChange>& changes);
		QStringList customIDsOfMessages(const QList<Message>& messages);

		// Takes lists of feeds/categories and assembles them into the tree structure.
		void assembleCategories(Assignment categories);
		void assembleFeeds(Assignment feeds);

	signals:
		// Emitted if data in any item belonging to this root are changed.
		void dataChanged(QList<RootItem*> items);
		void reloadMessageListRequested(bool mark_selected_messages_read);
		void itemExpandRequested(QList<RootItem*> items, bool expand);
		void itemExpandStateSaveRequested(RootItem* subtree_root);

		void itemReassignmentRequested(RootItem* item, RootItem* new_parent);
		void itemRemovalRequested(RootItem* item);

	private:
		virtual QMap<int, QVariant> storeCustomFeedsData() = 0;
		virtual void restoreCustomFeedsData(const QMap<int, QVariant>& data, const QHash<int, Feed*>& feeds) = 0;

		int m_accountId;
};

#endif // SERVICEROOT_H
