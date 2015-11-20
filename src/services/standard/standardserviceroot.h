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

#ifndef STANDARDSERVICEROOT_H
#define STANDARDSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include <QCoreApplication>
#include <QPair>


class StandardRecycleBin;
class StandardCategory;
class StandardFeed;
class FeedsImportExportModel;
class QMenu;

typedef QList<QPair<int, StandardCategory*> > CategoryAssignment;
typedef QPair<int, StandardCategory*> CategoryAssignmentItem;

typedef QList<QPair<int, StandardFeed*> > FeedAssignment;
typedef QPair<int, StandardFeed*> FeedAssignmentItem;

class StandardServiceRoot : public ServiceRoot {
    Q_OBJECT

  public:
    explicit StandardServiceRoot(bool load_from_db, FeedsModel *feeds_model, RootItem *parent = NULL);
    virtual ~StandardServiceRoot();

    // Start/stop root.
    void start();
    void stop();

    bool canBeEdited();
    bool canBeDeleted();
    QVariant data(int column, int role) const;

    // Return "add feed" and "add category" items.
    QList<QAction*> addItemMenu();

    // Return menu to be shown in "Services -> service" menu.
    QList<QAction*> serviceMenu();

    // Message stuff.
    bool loadMessagesForItem(RootItem *item, QSqlTableModel *model);

    bool onBeforeSetMessagesRead(RootItem *selected_item, QList<int> message_db_ids, ReadStatus read);
    bool onAfterSetMessagesRead(RootItem *selected_item, QList<int> message_db_ids, ReadStatus read);

    bool onBeforeSwitchMessageImportance(RootItem *selected_item, int message_db_id, Importance important);
    bool onAfterSwitchMessageImportance(RootItem *selected_item, int message_db_id, Importance important);

    // Returns all standard categories which are lying under given root node.
    // This does NOT include the root node even if the node is category.
    QHash<int,StandardCategory*> categoriesForItem(RootItem *root);

    // Returns all categories from this root, each pair
    // consists of ID of parent item and pointer to category.
    QHash<int,StandardCategory*> allCategories();

    // Returns context specific menu actions for given feed.
    QList<QAction*> getContextMenuForFeed(StandardFeed *feed);

    // Access to standard recycle bin.
    StandardRecycleBin *recycleBin() const;

    // Takes structure residing under given root item and adds feeds/categories from
    // it to active structure.
    // NOTE: This is used for import/export of the model.
    bool mergeImportExportModel(FeedsImportExportModel *model, QString &output_message);

    bool markFeedsReadUnread(QList<Feed*> items, ReadStatus read);
    bool markRecycleBinReadUnread(ReadStatus read);
    bool cleanFeeds(QList<Feed*> items, bool clean_read_only);

  public slots:
    void addNewCategory();
    void addNewFeed();
    void importFeeds();
    void exportFeeds();

  private:
    // Returns converted ids of given feeds
    // which are suitable as IN clause for SQL queries.
    QStringList textualFeedIds(const QList<Feed *> &feeds);

    void loadFromDatabase();

    // Takes lists of feeds/categories and assembles
    // them into the tree structure.
    void assembleCategories(CategoryAssignment categories);
    void assembleFeeds(FeedAssignment feeds);

    StandardRecycleBin *m_recycleBin;

    // Menus.
    QAction *m_actionExportFeeds;
    QAction *m_actionImportFeeds;

    QList<QAction*> m_serviceMenu;
    QList<QAction*> m_addItemMenu;
    QList<QAction*> m_feedContextMenu;

    QAction *m_actionFeedFetchMetadata;
};

#endif // STANDARDSERVICEROOT_H
