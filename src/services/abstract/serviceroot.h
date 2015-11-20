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

#include "core/rootitem.h"

#include "core/message.h"


class FeedsModel;
class QAction;
class QSqlTableModel;

// THIS IS the root node of the service.
// NOTE: The root usually contains some core functionality of the
// service like service account username/password etc.
class ServiceRoot : public RootItem {
    Q_OBJECT

  public:
    explicit ServiceRoot(FeedsModel *feeds_model, RootItem *parent = NULL);
    virtual ~ServiceRoot();

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

    // Start/stop services.
    // Start method is called when feed model gets initialized OR after user adds new service.
    //
    // Stop method is called just before application exits OR when
    // user explicitly deletes existing service instance.
    virtual void start() = 0;
    virtual void stop() = 0;

    // This method should prepare messages for given "item" (download them maybe?)
    // into predefined "Messages" table
    // and then use method QSqlTableModel::setFilter(....).
    // NOTE: It would be more preferable if all messages are downloaded
    // right when feeds are updated.
    // TODO: toto možná udělat asynchronně, zobrazit
    // "loading" dialog přes view a toto zavolat, nasledně signalovat
    virtual bool loadMessagesForItem(RootItem *item, QSqlTableModel *model) = 0;

    // Called BEFORE this read status update is stored in DB,
    // when false is returned, change is aborted.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.
    //
    // "read" is status which is ABOUT TO BE SET.
    virtual bool onBeforeSetMessagesRead(RootItem *selected_item, QList<int> message_db_ids, ReadStatus read) = 0;

    // Called AFTER this read status update is stored in DB,
    // when false is returned, change is aborted.
    // Here service root should inform (via signals)
    // which items are actually changed.
    //
    // "read" is status which is ABOUT TO BE SET.
    virtual bool onAfterSetMessagesRead(RootItem *selected_item, QList<int> message_db_ids, ReadStatus read) = 0;

    // Called BEFORE this importance switch update is stored in DB,
    // when false is returned, change is aborted.
    // This is the place to make some other changes like updating
    // some ONLINE service or something.
    //
    // "changes" - list of pairs - <message (integer id), new status>
    virtual bool onBeforeSwitchMessageImportance(RootItem *selected_item, QList<QPair<int,RootItem::Importance> > changes) = 0;

    // Called AFTER this importance switch update is stored in DB,
    // when false is returned, change is aborted.
    // Here service root should inform (via signals)
    // which items are actually changed.
    //
    // "changes" - list of pairs - <message (integer id), new status>
    virtual bool onAfterSwitchMessageImportance(RootItem *selected_item, QList<QPair<int,RootItem::Importance> > changes) = 0;

    // Access to feed model.
    FeedsModel *feedsModel() const;

    // Obvious method to wrap dataChanged(...) signal
    // which can be used by items themselves or any
    // other component.
    void itemChanged(RootItem *item);

  signals:
    // Emitted if data in any item belonging to this root are changed.
    void dataChanged(RootItem *item);
    void readFeedsFilterInvalidationRequested();

  private:
    FeedsModel *m_feedsModel;
};

#endif // SERVICEROOT_H
