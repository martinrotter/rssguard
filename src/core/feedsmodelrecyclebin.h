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

#ifndef FEEDSMODELRECYCLEBIN_H
#define FEEDSMODELRECYCLEBIN_H

#include "core/feedsmodelrootitem.h"

#include <QCoreApplication>


class FeedsModelRecycleBin : public FeedsModelRootItem {
    Q_DECLARE_TR_FUNCTIONS(FeedsModelRecycleBin)

  public:
    explicit FeedsModelRecycleBin(FeedsModelRootItem *parent = NULL);
    virtual ~FeedsModelRecycleBin();

    int childCount() const;
    void appendChild(FeedsModelRootItem *child);
    int countOfUnreadMessages() const;
    int countOfAllMessages() const;
    QVariant data(int column, int role) const;

    bool empty();
    bool restore();

  public slots:
    void updateCounts(bool update_total_count);

  private:
    int m_totalCount;
    int m_unreadCount;
};

#endif // FEEDSMODELRECYCLEBIN_H
