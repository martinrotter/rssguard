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

bool NextcloudFeed::deleteItem() {
  if (serviceRoot()->network()->deleteFeed(customId(), account()->networkProxy()) && removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

bool NextcloudFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, this, serviceRoot()->accountId());
}

NextcloudServiceRoot* NextcloudFeed::serviceRoot() const {
  return qobject_cast<NextcloudServiceRoot*>(account());
}
