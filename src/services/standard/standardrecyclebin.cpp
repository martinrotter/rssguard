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

#include "services/standard/standardrecyclebin.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/standard/standardserviceroot.h"

#include <QSqlQuery>


StandardRecycleBin::StandardRecycleBin(RootItem *parent)
  : RecycleBin(parent) {
  setId(ID_RECYCLE_BIN);
}

StandardRecycleBin::~StandardRecycleBin() {
  qDebug("Destroying RecycleBin instance.");
}

StandardServiceRoot *StandardRecycleBin::serviceRoot() {
  return static_cast<StandardServiceRoot*>(getParentServiceRoot());
}

int StandardRecycleBin::countOfUnreadMessages() const {
  return m_unreadCount;
}

int StandardRecycleBin::countOfAllMessages() const {
  return m_totalCount;
}

bool StandardRecycleBin::markAsReadUnread(RootItem::ReadStatus status) {
  return serviceRoot()->markRecycleBinReadUnread(status);
}

bool StandardRecycleBin::empty() {
  return RecycleBin::empty();
}

bool StandardRecycleBin::restore() {
  return RecycleBin::restore();
}

void StandardRecycleBin::updateCounts(bool update_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);
  query_all.setForwardOnly(true);

  if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE is_read = 0 AND is_deleted = 1 AND is_pdeleted = 0 AND account_id = %1;").arg(serviceRoot()->accountId())) && query_all.next()) {
    m_unreadCount = query_all.value(0).toInt();
  }
  else {
    m_unreadCount = 0;
  }

  if (update_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = %1;").arg(serviceRoot()->accountId())) && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
    else {
      m_totalCount = 0;
    }
  }
}
