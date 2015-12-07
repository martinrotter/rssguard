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

#include "services/abstract/serviceroot.h"

#include "core/feedsmodel.h"
#include "miscellaneous/application.h"

#include <QSqlQuery>


ServiceRoot::ServiceRoot(RootItem *parent) : RootItem(parent), m_accountId(NO_PARENT_CATEGORY) {
  setKind(RootItemKind::ServiceRoot);
}

ServiceRoot::~ServiceRoot() {
}

bool ServiceRoot::deleteViaGui() {
  QSqlDatabase connection = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  // Remove all messages.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM Messages WHERE account_id = %1;").arg(accountId()))) {
    return false;
  }

  // Remove all feeds.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM Feeds WHERE account_id = %1;").arg(accountId()))) {
    return false;
  }

  // Remove all categories.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM Categories WHERE account_id = %1;").arg(accountId()))) {
    return false;
  }

  // Switch "existence" flag.
  bool data_removed = QSqlQuery(connection).exec(QString("DELETE FROM Accounts WHERE id = %1;").arg(accountId()));

  if (data_removed) {
    requestItemRemoval(this);
  }

  return data_removed;
}

void ServiceRoot::itemChanged(QList<RootItem*> items) {
  emit dataChanged(items);
}

void ServiceRoot::requestReloadMessageList(bool mark_selected_messages_read) {
  emit reloadMessageListRequested(mark_selected_messages_read);
}

void ServiceRoot::requestFeedReadFilterReload() {
  emit readFeedsFilterInvalidationRequested();
}

void ServiceRoot::requestItemReassignment(RootItem *item, RootItem *new_parent) {
  emit itemReassignmentRequested(item, new_parent);
}

void ServiceRoot::requestItemRemoval(RootItem *item) {
  emit itemRemovalRequested(item);
}

int ServiceRoot::accountId() const {
  return m_accountId;
}

void ServiceRoot::setAccountId(int account_id) {
  m_accountId = account_id;
}
