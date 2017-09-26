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

#include "services/abstract/category.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"
#include "services/abstract/serviceroot.h"

Category::Category(RootItem* parent) : RootItem(parent) {
  setKind(RootItemKind::Category);

  if (icon().isNull()) {
    setIcon(qApp->icons()->fromTheme(QSL("folder")));
  }
}

Category::Category(const Category& other) : RootItem(other) {
  setKind(RootItemKind::Category);

  if (icon().isNull()) {
    setIcon(qApp->icons()->fromTheme(QSL("folder")));
  }
}

Category::Category(const QSqlRecord& record) : Category(nullptr) {
  setId(record.value(CAT_DB_ID_INDEX).toInt());
  setCustomId(record.value(CAT_DB_CUSTOM_ID_INDEX).toString());

  if (customId().isEmpty()) {
    setCustomId(QString::number(id()));
  }

  setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  setDescription(record.value(CAT_DB_DESCRIPTION_INDEX).toString());
  setCreationDate(TextFactory::parseDateTime(record.value(CAT_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());

  QIcon loaded_icon = qApp->icons()->fromByteArray(record.value(CAT_DB_ICON_INDEX).toByteArray());

  if (!loaded_icon.isNull()) {
    setIcon(loaded_icon);
  }
}

Category::~Category() {}

void Category::updateCounts(bool including_total_count) {
  QList<Feed*> feeds;

  foreach (RootItem* child, getSubTree()) {
    if (child->kind() == RootItemKind::Feed) {
      feeds.append(child->toFeed());
    }
    else if (child->kind() != RootItemKind::Category && child->kind() != RootItemKind::ServiceRoot) {
      child->updateCounts(including_total_count);
    }
  }

  if (feeds.isEmpty()) {
    return;
  }

  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  bool ok;

  QMap<QString, QPair<int, int>> counts = DatabaseQueries::getMessageCountsForCategory(database,
                                                                                       customId(),
                                                                                       getParentServiceRoot()->accountId(),
                                                                                       including_total_count,
                                                                                       &ok);

  if (ok) {
    foreach (Feed* feed, feeds) {
      if (counts.contains(feed->customId())) {
        feed->setCountOfUnreadMessages(counts.value(feed->customId()).first);

        if (including_total_count) {
          feed->setCountOfAllMessages(counts.value(feed->customId()).second);
        }
      }
    }
  }
}

bool Category::cleanMessages(bool clean_read_only) {
  return getParentServiceRoot()->cleanFeeds(getSubTreeFeeds(), clean_read_only);
}

bool Category::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  CacheForServiceRoot* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this), status);
  }

  return service->markFeedsReadUnread(getSubTreeFeeds(), status);
}
