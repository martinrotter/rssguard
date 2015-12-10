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

#include "services/abstract/recyclebin.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

#include <QSqlQuery>


RecycleBin::RecycleBin(RootItem *parent_item) : RootItem(parent_item) {
  setKind(RootItemKind::Bin);
  setIcon(qApp->icons()->fromTheme(QSL("folder-recycle-bin")));
  setTitle(tr("Recycle bin"));
  setDescription(tr("Recycle bin contains all deleted messages from all feeds."));
  setCreationDate(QDateTime::currentDateTime());
}

RecycleBin::~RecycleBin() {
}

QVariant RecycleBin::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      return tr("Recycle bin\n\n%1").arg(tr("%n deleted message(s).", 0, countOfAllMessages()));

    default:
      return RootItem::data(column, role);
  }
}

bool RecycleBin::empty() {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin emptying.");
    return false;
  }

  ServiceRoot *parent_root = getParentServiceRoot();
  QSqlQuery query_empty_bin(db_handle);

  query_empty_bin.setForwardOnly(true);
  query_empty_bin.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_deleted = 1 AND account_id = :account_id;"));
  query_empty_bin.bindValue(QSL(":account_id"), parent_root->accountId());

  if (!query_empty_bin.exec()) {
    qWarning("Query execution failed for recycle bin emptying.");

    db_handle.rollback();
    return false;
  }

  // Commit changes.
  if (db_handle.commit()) {
    updateCounts(true);
    parent_root->itemChanged(QList<RootItem*>() << this);
    parent_root->requestReloadMessageList(true);
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool RecycleBin::restore() {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin restoring.");
    return false;
  }

  ServiceRoot *parent_root = getParentServiceRoot();
  QSqlQuery query_empty_bin(db_handle);

  query_empty_bin.setForwardOnly(true);
  query_empty_bin.prepare("UPDATE Messages SET is_deleted = 0 "
                          "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  query_empty_bin.bindValue(QSL(":account_id"), parent_root->accountId());

  if (!query_empty_bin.exec()) {
    qWarning("Query execution failed for recycle bin restoring.");

    db_handle.rollback();
    return false;
  }

  // Commit changes.
  if (db_handle.commit()) {
    parent_root->updateCounts(true);
    parent_root->itemChanged(parent_root->getSubTree());
    parent_root->requestReloadMessageList(true);
    parent_root->requestFeedReadFilterReload();
    return true;
  }
  else {
    return db_handle.rollback();
  }
}
