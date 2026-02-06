// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/nextcloudfeed.h"

#include "src/nextcloudnetworkfactory.h"
#include "src/nextcloudserviceroot.h"

#include <librssguard/database/databasequeries.h>

#include <QPointer>

NextcloudFeed::NextcloudFeed(RootItem* parent) : Feed(parent) {}

bool NextcloudFeed::canBeDeleted() const {
  return true;
}

void NextcloudFeed::deleteItem() {
  serviceRoot()->network()->deleteFeed(customId(), account()->networkProxy());
  removeItself();
  serviceRoot()->requestItemRemoval(this, false);
}

void NextcloudFeed::removeItself() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteFeed(db, this, serviceRoot()->accountId());
  });
}

NextcloudServiceRoot* NextcloudFeed::serviceRoot() const {
  return qobject_cast<NextcloudServiceRoot*>(account());
}
