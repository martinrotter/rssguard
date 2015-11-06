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


class FeedsModel;

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
    // NOTE: This method should always create new actions in memory
    // before returning them because caller takes ownership of any
    // actions returned from here.
    virtual QList<QAction*> specificAddItemActions() = 0;

    inline FeedsModel *feedsModel() const {
      return m_feedsModel;
    }

  protected:
    FeedsModel *m_feedsModel;
};

#endif // SERVICEROOT_H
