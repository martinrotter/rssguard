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

#ifndef TTRSSSERVICEROOT_H
#define TTRSSSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include <QCoreApplication>


class TtRssCategory;
class TtRssFeed;
class TtRssNetworkFactory;

class TtRssServiceRoot : public ServiceRoot {
    Q_OBJECT

  public:
    explicit TtRssServiceRoot(RootItem *parent = NULL);
    virtual ~TtRssServiceRoot();

    void start();
    void stop();

    QString code();

    bool canBeEdited();
    bool canBeDeleted();
    bool editViaGui();
    bool deleteViaGui();

    QVariant data(int column, int role) const;

    QList<QAction*> addItemMenu();
    QList<QAction*> serviceMenu();
    QList<QAction*> contextMenu();

    RecycleBin *recycleBin();

    bool loadMessagesForItem(RootItem *item, QSqlTableModel *model);

    bool onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, ReadStatus read);
    bool onAfterSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, ReadStatus read);

    bool onBeforeSwitchMessageImportance(RootItem *selected_item, const QList<QPair<Message,RootItem::Importance> > &changes);
    bool onAfterSwitchMessageImportance(RootItem *selected_item, const QList<QPair<Message,RootItem::Importance> > &changes);

    bool onBeforeMessagesDelete(RootItem *selected_item, QList<int> message_db_ids);
    bool onAfterMessagesDelete(RootItem *selected_item, QList<int> message_db_ids);

    bool onBeforeMessagesRestoredFromBin(RootItem *selected_item, QList<int> message_db_ids);
    bool onAfterMessagesRestoredFromBin(RootItem *selected_item, QList<int> message_db_ids);

    TtRssNetworkFactory *network() const;

    void saveAccountDataToDatabase();
    void updateTitle();

    void completelyRemoveAllData();

  public slots:
    void syncIn();

  private:
    QStringList customIDsOfMessages(const QList<QPair<Message,Importance> > &changes);
    QStringList customIDsOfMessages(const QList<Message> &messages);

    // Returns converted ids of given feeds
    // which are suitable as IN clause for SQL queries.
    QStringList textualFeedIds(const QList<Feed*> &feeds);

    void removeOldFeedTree(bool including_messages);
    void cleanAllItems();
    void storeNewFeedTree(RootItem *root);
    void loadFromDatabase();

    QAction *m_actionSyncIn;
    QList<QAction*> m_serviceMenu;

    TtRssNetworkFactory *m_network;
};

#endif // TTRSSSERVICEROOT_H
