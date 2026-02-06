// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/standardcategory.h"

#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/services/abstract/gui/formcategorydetails.h>

#include <QPointer>

StandardCategory::StandardCategory(RootItem* parent_item) : Category(parent_item) {}

StandardServiceRoot* StandardCategory::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(account());
}

Qt::ItemFlags StandardCategory::additionalFlags() const {
  return Category::additionalFlags() | Qt::ItemFlag::ItemIsDragEnabled | Qt::ItemFlag::ItemIsDropEnabled;
}

bool StandardCategory::performDragDropChange(RootItem* target_item) {
  try {
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      DatabaseQueries::createOverwriteCategory(db, this, account()->accountId(), target_item->id());
    });

    serviceRoot()->requestItemReassignment(this, target_item);
    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Cannot overwrite category:" << QUOTE_W_SPACE_DOT(ex.message());
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot save category data"),
                          tr("Cannot save data for category, detailed information was logged via debug log."),
                          QSystemTrayIcon::MessageIcon::Critical});
    return false;
  }
}

bool StandardCategory::canBeEdited() const {
  return true;
}

bool StandardCategory::canBeDeleted() const {
  return true;
}

void StandardCategory::deleteItem() {
  removeItself();
  serviceRoot()->requestItemRemoval(this, false);
}

void StandardCategory::removeItself() {
  // Remove all child items (feeds and categories)
  // from the database.
  auto chi = childItems();

  for (RootItem* child : std::as_const(chi)) {
    if (child->kind() == RootItem::Kind::Category) {
      qobject_cast<StandardCategory*>(child)->removeItself();
    }
    else if (child->kind() == RootItem::Kind::Feed) {
      qobject_cast<StandardFeed*>(child)->removeItself();
    }
  }

  // Children are removed, remove this standard category too.
  qApp->database()->worker()->read([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteCategory(db, this);
  });
}
