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

#include "core/feedsmodelstandardcategory.h"

#include "core/defs.h"
#include "core/databasefactory.h"
#include "core/textfactory.h"
#include "core/settings.h"
#include "gui/iconthemefactory.h"
#include "gui/iconfactory.h"

#include <QVariant>
#include <QSqlQuery>


FeedsModelStandardCategory::FeedsModelStandardCategory(FeedsModelRootItem *parent_item)
  : FeedsModelCategory(parent_item) {
  m_type = Standard;
}

FeedsModelStandardCategory::~FeedsModelStandardCategory() {
  qDebug("Destroying FeedsModelStandardCategory instance.");
}

QVariant FeedsModelStandardCategory::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        //: Tooltip for standard feed.
        return tr("%1 (standard category)\n"
                  "%2%3").arg(m_title,
                              m_description,
                              m_childItems.size() == 0 ?
                                tr("\n\nThis category does not contain any nested items.") :
                                "");
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread message(s).", "", countOfUnreadMessages());
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

    case Qt::ForegroundRole:
      return countOfUnreadMessages() > 0 ? QColor(0, 64, 255) : QVariant();

    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return Settings::instance()->value(APP_CFG_FEEDS,
                                           "count_format",
                                           "(%unread)").toString()
            .replace("%unread", QString::number(countOfUnreadMessages()))
            .replace("%all", QString::number(countOfAllMessages()));
      }
      else {
        return QVariant();
      }

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_icon;
      }
      else {
        return QVariant();
      }

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

bool FeedsModelStandardCategory::removeItself() {
  bool result = true;

  // Remove all child items (feeds, categories.)
  foreach (FeedsModelRootItem *child, m_childItems) {
    result &= child->removeItself();
  }

  if (!result) {
    return result;
  }

  // Children are removed, remove this standard category too.
  QSqlDatabase database = DatabaseFactory::instance()->connection("FeedsModelStandardCategory",
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_remove(database);

  query_remove.setForwardOnly(true);

  // Remove all messages from this standard feed.
  query_remove.prepare("DELETE FROM Categories WHERE id = :category;");
  query_remove.bindValue(":category", id());

  return query_remove.exec();
}

FeedsModelStandardCategory *FeedsModelStandardCategory::loadFromRecord(const QSqlRecord &record) {
  FeedsModelStandardCategory *category = new FeedsModelStandardCategory(NULL);

  category->setId(record.value(CAT_DB_ID_INDEX).toInt());
  category->setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  category->setDescription(record.value(CAT_DB_DESCRIPTION_INDEX).toString());
  category->setCreationDate(TextFactory::parseDateTime(record.value(CAT_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  category->setIcon(IconFactory::fromByteArray(record.value(CAT_DB_ICON_INDEX).toByteArray()));

  return category;
}
