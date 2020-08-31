// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardcategory.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/gui/formstandardcategorydetails.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardserviceroot.h"

#include <QPointer>

StandardCategory::StandardCategory(RootItem* parent_item) : Category(parent_item) {}

StandardServiceRoot* StandardCategory::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(getParentServiceRoot());
}

Qt::ItemFlags StandardCategory::additionalFlags() const {
  return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool StandardCategory::performDragDropChange(RootItem* target_item) {
  auto* category_new = new StandardCategory(*this);

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

bool StandardCategory::canBeEdited() const {
  return true;
}

bool StandardCategory::canBeDeleted() const {
  return true;
}

bool StandardCategory::editViaGui() {
  QScopedPointer<FormStandardCategoryDetails> form_pointer(new FormStandardCategoryDetails(serviceRoot(),
                                                                                           qApp->mainFormWidget()));

  form_pointer->addEditCategory(this, nullptr);
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

bool StandardCategory::removeItself() {
  bool children_removed = true;

  // Remove all child items (feeds and categories)
  // from the database.
  for (RootItem* child : childItems()) {
    if (child->kind() == RootItem::Kind::Category) {
      children_removed &= dynamic_cast<StandardCategory*>(child)->removeItself();
    }
    else if (child->kind() == RootItem::Kind::Feed) {
      children_removed &= dynamic_cast<StandardFeed*>(child)->removeItself();
    }
  }

  if (children_removed) {
    // Children are removed, remove this standard category too.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className());

    return DatabaseQueries::deleteStandardCategory(database, id());
  }
  else {
    return false;
  }
}

bool StandardCategory::addItself(RootItem* parent) {
  // Now, add category to persistent storage.
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  int new_id = DatabaseQueries::addStandardCategory(database, parent->id(), parent->getParentServiceRoot()->accountId(),
                                                    title(), description(), creationDate(), icon());

  if (new_id <= 0) {
    return false;
  }
  else {
    setId(new_id);
    setCustomId(QString::number(new_id));
    return true;
  }
}

bool StandardCategory::editItself(StandardCategory* new_category_data) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  StandardCategory* original_category = this;
  RootItem* new_parent = new_category_data->parent();

  if (DatabaseQueries::editStandardCategory(database, new_parent->id(), original_category->id(),
                                            new_category_data->title(), new_category_data->description(),
                                            new_category_data->icon())) {
    // Setup new model data for the original item.
    original_category->setDescription(new_category_data->description());
    original_category->setIcon(new_category_data->icon());
    original_category->setTitle(new_category_data->title());

    // Editing is done.
    return true;
  }
  else {
    return false;
  }
}

StandardCategory::StandardCategory(const QSqlRecord& record) : Category(record) {}
