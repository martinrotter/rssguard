// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "services/standard/standardserviceroot.h"
#include "services/standard/standardfeed.h"

#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QPointer>


StandardCategory::StandardCategory(RootItem *parent_item) : Category(parent_item) {
}

StandardCategory::StandardCategory(const StandardCategory &other)
  : Category(NULL) {
  setId(other.id());
  setCustomId(other.customId());
  setTitle(other.title());
  setDescription(other.description());
  setIcon(other.icon());
  setCreationDate(other.creationDate());
  setChildItems(other.childItems());
  setParent(other.parent());
}

StandardCategory::~StandardCategory() {
  qDebug("Destroying Category instance.");
}

StandardServiceRoot *StandardCategory::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(getParentServiceRoot());
}

QVariant StandardCategory::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        //: Tooltip for standard feed.
        return tr("%1 (category)"
                  "%2%3").arg(title(),
                              description().isEmpty() ? QString() : QSL("\n") + description(),
                              childCount() == 0 ?
                                tr("\nThis category does not contain any nested items.") :
                                QString());
      }
      else {
        return Category::data(column, role);
      }

    default:
      return Category::data(column, role);
  }
}

Qt::ItemFlags StandardCategory::additionalFlags() const {
  return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool StandardCategory::performDragDropChange(RootItem *target_item) {
  StandardCategory *category_new = new StandardCategory(*this);
  category_new->clearChildren();
  category_new->setParent(target_item);

  if (editItself(category_new)) {
    serviceRoot()->requestItemReassignment(this, target_item);
    delete category_new;
    return true;
  }
  else {
    delete category_new;
    return false;
  }
}

bool StandardCategory::editViaGui() {
  QScopedPointer<FormStandardCategoryDetails> form_pointer(new FormStandardCategoryDetails(serviceRoot(), qApp->mainForm()));

  form_pointer.data()->exec(this, NULL);
  return false;
}

bool StandardCategory::deleteViaGui() {
  if (removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

bool StandardCategory::markAsReadUnread(ReadStatus status) {
  return serviceRoot()->markFeedsReadUnread(getSubTreeFeeds(), status);
}

bool StandardCategory::cleanMessages(bool clean_read_only) {
  return serviceRoot()->cleanFeeds(getSubTreeFeeds(), clean_read_only);
}

bool StandardCategory::removeItself() {
  bool children_removed = true;

  // Remove all child items (feeds and categories)
  // from the database.
  foreach (RootItem *child, childItems()) {
    if (child->kind() == RootItemKind::Category) {
      children_removed &= static_cast<StandardCategory*>(child)->removeItself();
    }
    else if (child->kind() == RootItemKind::Feed) {
      children_removed &= static_cast<StandardFeed*>(child)->removeItself();
    }
  }

  if (children_removed) {
    // Children are removed, remove this standard category too.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
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
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_add(database);

  query_add.setForwardOnly(true);
  query_add.prepare("INSERT INTO Categories "
                    "(parent_id, title, description, date_created, icon, account_id) "
                    "VALUES (:parent_id, :title, :description, :date_created, :icon, :account_id);");
  query_add.bindValue(QSL(":parent_id"), parent->id());
  query_add.bindValue(QSL(":title"), title());
  query_add.bindValue(QSL(":description"), description());
  query_add.bindValue(QSL(":date_created"), creationDate().toMSecsSinceEpoch());
  query_add.bindValue(QSL(":icon"), qApp->icons()->toByteArray(icon()));
  query_add.bindValue(QSL(":account_id"), parent->getParentServiceRoot()->accountId());

  if (!query_add.exec()) {
    qDebug("Failed to add category to database: '%s'.", qPrintable(query_add.lastError().text()));

    // Query failed.
    return false;
  }

  setId(query_add.lastInsertId().toInt());
  setCustomId(id());

  // Now set custom ID in the DB.
  query_add.prepare(QSL("UPDATE Categories SET custom_id = :custom_id WHERE id = :id;"));
  query_add.bindValue(QSL(":custom_id"), QString::number(customId()));
  query_add.bindValue(QSL(":id"), id());
  query_add.exec();

  return true;
}

bool StandardCategory::editItself(StandardCategory *new_category_data) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
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

StandardCategory::StandardCategory(const QSqlRecord &record) : Category(NULL) {
  setId(record.value(CAT_DB_ID_INDEX).toInt());
  setCustomId(id());
  setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  setDescription(record.value(CAT_DB_DESCRIPTION_INDEX).toString());
  setCreationDate(TextFactory::parseDateTime(record.value(CAT_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  setIcon(qApp->icons()->fromByteArray(record.value(CAT_DB_ICON_INDEX).toByteArray()));
}
