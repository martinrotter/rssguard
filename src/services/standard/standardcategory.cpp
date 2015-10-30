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

#include "services/standard/standardcategory.h"

#include "definitions/definitions.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"
#include "core/feedsmodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "services/standard/gui/formstandardcategorydetails.h"

#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QPointer>


StandardCategory::StandardCategory(RootItem *parent_item) : RootItem(parent_item) {
  init();
}

StandardCategory::StandardCategory(const StandardCategory &other)
  : RootItem(NULL) {
  m_kind = other.kind();
  m_id = other.id();
  m_title = other.title();
  m_description = other.description();
  m_icon = other.icon();
  m_creationDate = other.creationDate();
  m_childItems = other.childItems();
  m_parentItem = other.parent();
}

StandardCategory::~StandardCategory() {
  qDebug("Destroying Category instance.");
}

void StandardCategory::init() {
  m_kind = RootItem::Cattegory;
}

QVariant StandardCategory::data(int column, int role) const {
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
        return qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString()
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

void StandardCategory::edit() {
  // TODO: fix passing of the model
  QPointer<FormStandardCategoryDetails> form_pointer = new FormStandardCategoryDetails(qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel(),
                                                                                       qApp->mainForm());

  form_pointer.data()->exec(this, NULL);

  delete form_pointer.data();
}

bool StandardCategory::removeItself() {
  bool children_removed = true;

  // Remove all child items (feeds, categories.)
  foreach (RootItem *child, m_childItems) {
    children_removed &= child->removeItself();
  }

  if (children_removed) {
    // Children are removed, remove this standard category too.
    QSqlDatabase database = qApp->database()->connection(QSL("Category"), DatabaseFactory::FromSettings);
    QSqlQuery query_remove(database);

    // Remove this category from database.
    query_remove.setForwardOnly(true);
    query_remove.prepare(QSL("DELETE FROM Categories WHERE id = :category;"));
    query_remove.bindValue(QSL(":category"), id());

    return query_remove.exec();
  }
  else {
    return false;
  }
}

bool StandardCategory::addItself(RootItem *parent) {
  // Now, add category to persistent storage.
  // Children are removed, remove this standard category too.
  QSqlDatabase database = qApp->database()->connection(QSL("Category"), DatabaseFactory::FromSettings);
  QSqlQuery query_add(database);

  query_add.setForwardOnly(true);
  query_add.prepare("INSERT INTO Categories "
                    "(parent_id, title, description, date_created, icon) "
                    "VALUES (:parent_id, :title, :description, :date_created, :icon);");
  query_add.bindValue(QSL(":parent_id"), parent->id());
  query_add.bindValue(QSL(":title"), title());
  query_add.bindValue(QSL(":description"), description());
  query_add.bindValue(QSL(":date_created"), creationDate().toMSecsSinceEpoch());
  query_add.bindValue(QSL(":icon"), qApp->icons()->toByteArray(icon()));

  if (!query_add.exec()) {
    qDebug("Failed to add category to database: %s.", qPrintable(query_add.lastError().text()));

    // Query failed.
    return false;
  }

  query_add.prepare(QSL("SELECT id FROM Categories WHERE title = :title;"));
  query_add.bindValue(QSL(":title"), title());

  if (query_add.exec() && query_add.next()) {
    // New category was added, fetch is primary id
    // from the database.
    setId(query_add.value(0).toInt());
  }
  else {
    // Something failed.
    return false;
  }

  return true;
}

bool StandardCategory::editItself(StandardCategory *new_category_data) {
  QSqlDatabase database = qApp->database()->connection(QSL("Category"), DatabaseFactory::FromSettings);
  QSqlQuery query_update_category(database);
  StandardCategory *original_category = this;
  RootItem *new_parent = new_category_data->parent();

  query_update_category.setForwardOnly(true);
  query_update_category.prepare("UPDATE Categories "
                                "SET title = :title, description = :description, icon = :icon, parent_id = :parent_id "
                                "WHERE id = :id;");
  query_update_category.bindValue(QSL(":title"), new_category_data->title());
  query_update_category.bindValue(QSL(":description"), new_category_data->description());
  query_update_category.bindValue(QSL(":icon"), qApp->icons()->toByteArray(new_category_data->icon()));
  query_update_category.bindValue(QSL(":parent_id"), new_parent->id());
  query_update_category.bindValue(QSL(":id"), original_category->id());

  if (!query_update_category.exec()) {
    // Persistent storage update failed, no way to continue now.
    return false;
  }

  // Setup new model data for the original item.
  original_category->setDescription(new_category_data->description());
  original_category->setIcon(new_category_data->icon());
  original_category->setTitle(new_category_data->title());

  // Editing is done.
  return true;
}

StandardCategory::StandardCategory(const QSqlRecord &record) : RootItem(NULL) {
  init();

  setId(record.value(CAT_DB_ID_INDEX).toInt());
  setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  setDescription(record.value(CAT_DB_DESCRIPTION_INDEX).toString());
  setCreationDate(TextFactory::parseDateTime(record.value(CAT_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  setIcon(qApp->icons()->fromByteArray(record.value(CAT_DB_ICON_INDEX).toByteArray()));
}
