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

    bool canBeEdited();
    bool canBeDeleted();
    QVariant data(int column, int role) const;

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

    // Return "add feed" and "add category" items.
    QList<QAction*> addItemMenu();

  private:
    void loadFromDatabase();

    // Takes lists of feeds/categories and assembles
    // them into the tree structure.
    void assembleCategories(CategoryAssignment categories);
    void assembleFeeds(FeedAssignment feeds);

    StandardRecycleBin *m_recycleBin;

    // Menus.
    QList<QAction*> m_addItemMenu;
    QList<QAction*> m_feedContextMenu;

    QAction *m_actionFeedFetchMetadata;
};

#endif // STANDARDSERVICEROOT_H
