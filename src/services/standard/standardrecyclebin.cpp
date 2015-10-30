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

#include <QSqlQuery>


StandardRecycleBin::StandardRecycleBin(RootItem *parent)
  : RootItem(parent) {
  m_kind = RootItem::Bin;
  m_icon = qApp->icons()->fromTheme(QSL("folder-recycle-bin"));
  m_id = ID_RECYCLE_BIN;
  m_title = tr("Recycle bin");
  m_description = tr("Recycle bin contains all deleted messages from all feeds.");
  m_creationDate = QDateTime::currentDateTime();

  updateCounts(true);
}

StandardRecycleBin::~StandardRecycleBin() {
  qDebug("Destroying RecycleBin instance.");
}

int StandardRecycleBin::childCount() const {
  return 0;
}

void StandardRecycleBin::appendChild(RootItem *child) {
  Q_UNUSED(child)
}

int StandardRecycleBin::countOfUnreadMessages() const {
  return m_unreadCount;
}

int StandardRecycleBin::countOfAllMessages() const {
  return m_totalCount;
}

QVariant StandardRecycleBin::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString()
            .replace(PLACEHOLDER_UNREAD_COUNTS, QString::number(countOfUnreadMessages()))
            .replace(PLACEHOLDER_ALL_COUNTS, QString::number(countOfAllMessages()));
      }
      else {
        return QVariant();
      }

    case Qt::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_icon;
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      return tr("Recycle bin\n%1").arg(tr("%n deleted message(s).", 0, countOfAllMessages()));

    case Qt::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}

bool StandardRecycleBin::empty() {
  QSqlDatabase db_handle = qApp->database()->connection(QSL("RecycleBin"), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin emptying.");
    return false;
  }

  QSqlQuery query_empty_bin(db_handle);
  query_empty_bin.setForwardOnly(true);

  if (!query_empty_bin.exec(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_deleted = 1;"))) {
    qWarning("Query execution failed for recycle bin emptying.");

    db_handle.rollback();
    return false;
  }

  // Commit changes.
  if (db_handle.commit()) {
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool StandardRecycleBin::restore() {
  QSqlDatabase db_handle = qApp->database()->connection(QSL("RecycleBin"), DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for recycle bin restoring.");
    return false;
  }

  QSqlQuery query_empty_bin(db_handle);
  query_empty_bin.setForwardOnly(true);

  if (!query_empty_bin.exec(QSL("UPDATE Messages SET is_deleted = 0 WHERE is_deleted = 1 AND is_pdeleted = 0;"))) {
    qWarning("Query execution failed for recycle bin restoring.");

    db_handle.rollback();
    return false;
  }

  // Commit changes.
  if (db_handle.commit()) {
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

void StandardRecycleBin::updateCounts(bool update_total_count) {
  QSqlDatabase database = qApp->database()->connection(QSL("RecycleBin"), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);
  query_all.setForwardOnly(true);

  if (query_all.exec(QSL("SELECT count(*) FROM Messages WHERE is_read = 0 AND is_deleted = 1 AND is_pdeleted = 0;")) && query_all.next()) {
    m_unreadCount = query_all.value(0).toInt();
  }
  else {
    m_unreadCount = 0;
  }

  if (update_total_count) {
    if (query_all.exec(QSL("SELECT count(*) FROM Messages WHERE is_deleted = 1 AND is_pdeleted = 0;")) && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
    else {
      m_totalCount = 0;
    }
  }
}
