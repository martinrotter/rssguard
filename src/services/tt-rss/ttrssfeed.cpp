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

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"

#include <QSqlQuery>


TtRssFeed::TtRssFeed(RootItem *parent)
  : Feed(parent), m_customId(NO_PARENT_CATEGORY), m_totalCount(0), m_unreadCount(0) {
}

TtRssFeed::TtRssFeed(const QSqlRecord &record) : Feed(NULL), m_totalCount(0), m_unreadCount(0) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
}

TtRssFeed::~TtRssFeed() {
}

TtRssServiceRoot *TtRssFeed::serviceRoot() {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

void TtRssFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);

  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = '%1' AND is_deleted = 0 AND account_id = %2;").arg(QString::number(customId()),
                                                                                                                              QString::number(serviceRoot()->accountId()))) && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = '%1' AND is_deleted = 0 AND is_read = 0 AND account_id = %2;").arg(QString::number(customId()),
                                                                                                                                            QString::number(serviceRoot()->accountId()))) && query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (status() == NewMessages && new_unread_count < m_unreadCount) {
      setStatus(Normal);
    }

    m_unreadCount = new_unread_count;
  }
}

int TtRssFeed::countOfAllMessages() {
  return m_totalCount;
}

int TtRssFeed::countOfUnreadMessages() {
  return m_unreadCount;
}

int TtRssFeed::update() {
  // TODO: přes getHeadlines provede stažení kompletnich zprav.
  return 0;
}

QList<Message> TtRssFeed::undeletedMessages() const {
  return QList<Message>();
}

int TtRssFeed::customId() const {
  return m_customId;
}

void TtRssFeed::setCustomId(int custom_id) {
  m_customId = custom_id;
}
