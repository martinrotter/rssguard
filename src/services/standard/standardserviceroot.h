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

typedef QList<QPair<int, StandardCategory*> > CategoryAssignment;
typedef QPair<int, StandardCategory*> CategoryAssignmentItem;

typedef QList<QPair<int, StandardFeed*> > FeedAssignment;
typedef QPair<int, StandardFeed*> FeedAssignmentItem;

class StandardServiceRoot : public ServiceRoot {
    Q_DECLARE_TR_FUNCTIONS(StandardServiceRoot)

  public:
    explicit StandardServiceRoot(RootItem *parent = NULL);
    virtual ~StandardServiceRoot();

    bool canBeEdited();
    bool canBeDeleted();
    QVariant data(int column, int role) const;

    // Returns all standard categories which are lying under given root node.
    // This does NOT include the root node even if the node is category.
    QHash<int, StandardCategory*> categoriesForItem(RootItem *root);

    // Access to standard recycle bin.
    StandardRecycleBin *recycleBin() const;

  private:
    void loadFromDatabase();

    // Takes lists of feeds/categories and assembles
    // them into the tree structure.
    void assembleCategories(CategoryAssignment categories);
    void assembleFeeds(FeedAssignment feeds);

    StandardRecycleBin *m_recycleBin;
};

#endif // STANDARDSERVICEROOT_H
