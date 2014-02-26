// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/feedsmodelfeed.h"

#include "core/databasefactory.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>


FeedsModelFeed::FeedsModelFeed(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item),
    m_type(StandardRss0X),
    m_totalCount(0),
    m_unreadCount(0) {
  m_kind = FeedsModelRootItem::Feed;
}

FeedsModelFeed::~FeedsModelFeed() {
}

int FeedsModelFeed::childCount() const {
  // Because feed has no children.
  return 0;
}

int FeedsModelFeed::countOfAllMessages() const {
  return m_totalCount;
}

int FeedsModelFeed::countOfUnreadMessages() const {
  return m_unreadCount;
}

QString FeedsModelFeed::typeToString(FeedsModelFeed::Type type) {
  switch (type) {
    case StandardAtom10:
      return "ATOM 1.0";

    case StandardRdf:
      return "RDF";

    case StandardRss0X:
      return "RSS 0.91/0.92/0.93";

    case StandardRss2X:
    default:
      return "RSS 2.0/2.0.1";
  }
}

void FeedsModelFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = DatabaseFactory::instance()->connection("FeedsModelFeed",
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);
  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages "
                               "WHERE feed = %1 AND is_deleted = 0;").arg(id())) &&
        query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count(*) FROM Messages "
                             "WHERE feed = %1 AND is_deleted = 0 AND is_read = 0;").arg(id())) &&
      query_all.next()) {
    m_unreadCount = query_all.value(0).toInt();
  }
}



