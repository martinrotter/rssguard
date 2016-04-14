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

#include "services/abstract/category.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "services/abstract/serviceroot.h"
#include "services/abstract/feed.h"


Category::Category(RootItem *parent) : RootItem(parent) {
  setKind(RootItemKind::Category);
}

Category::~Category() {
}

void Category::updateCounts(bool including_total_count) {
  QList<Feed*> feeds;

  foreach (RootItem *child, childItems()) {
    if (child->kind() == RootItemKind::Feed) {
      feeds.append(child->toFeed());
    }
    else {
      child->updateCounts(including_total_count);
    }
  }

  if (feeds.isEmpty()) {
    return;
  }

  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  bool ok;
  QMap<int,QPair<int,int> > counts = DatabaseQueries::getMessageCountsForCategory(database, customId(), getParentServiceRoot()->accountId(),
                                                                                  including_total_count, &ok);

  if (ok) {
    foreach (Feed *feed, feeds) {
      if (counts.contains(feed->customId())) {
        feed->setCountOfUnreadMessages(counts.value(feed->customId()).first);

        if (including_total_count) {
          feed->setCountOfAllMessages(counts.value(feed->customId()).second);
        }
      }
    }
  }
}
