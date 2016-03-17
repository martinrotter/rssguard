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
#include "services/abstract/serviceroot.h"
#include "services/abstract/feed.h"

#include <QSqlQuery>


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
  QSqlQuery query_all(database);
  QMap<int,int> counts;

  query_all.setForwardOnly(true);

  if (including_total_count) {
    query_all.prepare("SELECT feed, count(*) FROM Messages "
                      "WHERE feed IN (SELECT custom_id FROM Feeds WHERE category = :category AND account_id = :account_id) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
                      "GROUP BY feed;");
    query_all.bindValue(QSL(":category"), customId());
    query_all.bindValue(QSL(":account_id"), getParentServiceRoot()->accountId());

    if (query_all.exec()) {
      while (query_all.next()) {
        int feed_id = query_all.value(0).toInt();
        int new_count = query_all.value(1).toInt();

        counts.insert(feed_id, new_count);
      }

      foreach (Feed *feed, feeds) {
        feed->setCountOfAllMessages(counts.value(feed->customId()));
      }
    }
  }

  counts.clear();
  query_all.prepare("SELECT feed, count(*) FROM Messages "
                    "WHERE feed IN (SELECT custom_id FROM Feeds WHERE category = :category AND account_id = :account_id) AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 0 AND account_id = :account_id "
                    "GROUP BY feed;");
  query_all.bindValue(QSL(":category"), customId());
  query_all.bindValue(QSL(":account_id"), getParentServiceRoot()->accountId());

  // Obtain count of unread messages.
  if (query_all.exec()) {
    while (query_all.next()) {
      int feed_id = query_all.value(0).toInt();
      int new_count = query_all.value(1).toInt();

      counts.insert(feed_id, new_count);
    }

    foreach (Feed *feed, feeds) {
      feed->setCountOfUnreadMessages(counts.value(feed->customId()));
    }
  }
}
