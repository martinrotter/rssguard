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

#include "core/feedsmodelcategory.h"

#include "definitions/definitions.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"

#include <QVariant>
#include <QSqlQuery>


FeedsModelCategory::FeedsModelCategory(FeedsModelRootItem *parent_item) : FeedsModelRootItem(parent_item) {
  m_kind = FeedsModelRootItem::Category;
}

FeedsModelCategory::FeedsModelCategory(const FeedsModelCategory &other)
  : FeedsModelRootItem(NULL) {
  m_kind = other.kind();
  m_id = other.id();
  m_title = other.title();
  m_description = other.description();
  m_icon = other.icon();
  m_creationDate = other.creationDate();
  m_childItems = other.childItems();
  m_parentItem = other.parent();
}

FeedsModelCategory::~FeedsModelCategory() {
  qDebug("Destroying FeedsModelCategory instance.");
}

QVariant FeedsModelCategory::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        //: Tooltip for standard feed.
        return tr("%1 (category)"
                  "%2%3").arg(m_title,
                              m_description.isEmpty() ? QString() : QString('\n') + m_description,
                              m_childItems.size() == 0 ?
                                tr("\nThis category does not contain any nested items.") :
                                QString());
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

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return qApp->settings()->value(APP_CFG_FEEDS, SETTING(Feeds::CountFormat)).toString()
            .replace(PLACEHOLDER_UNREAD_COUNTS, QString::number(countOfUnreadMessages()))
            .replace(PLACEHOLDER_ALL_COUNTS, QString::number(countOfAllMessages()));
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

bool FeedsModelCategory::removeItself() {
  bool result = true;

  // Remove all child items (feeds, categories.)
  foreach (FeedsModelRootItem *child, m_childItems) {
    result &= child->removeItself();
  }

  if (!result) {
    return result;
  }

  // Children are removed, remove this standard category too.
  QSqlDatabase database = qApp->database()->connection("FeedsModelCategory", DatabaseFactory::FromSettings);
  QSqlQuery query_remove(database);

  query_remove.setForwardOnly(true);

  // Remove all messages from this standard feed.
  query_remove.prepare("DELETE FROM Categories WHERE id = :category;");
  query_remove.bindValue(":category", id());

  return query_remove.exec();
}

FeedsModelCategory *FeedsModelCategory::loadFromRecord(const QSqlRecord &record) {
  FeedsModelCategory *category = new FeedsModelCategory(NULL);

  category->setId(record.value(CAT_DB_ID_INDEX).toInt());
  category->setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  category->setDescription(record.value(CAT_DB_DESCRIPTION_INDEX).toString());
  category->setCreationDate(TextFactory::parseDateTime(record.value(CAT_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  category->setIcon(qApp->icons()->fromByteArray(record.value(CAT_DB_ICON_INDEX).toByteArray()));

  return category;
}
