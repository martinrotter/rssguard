// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/owncloudfeed.h"

#include "database/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/owncloud/owncloudnetworkfactory.h"
#include "services/owncloud/owncloudserviceroot.h"

#include <QPointer>

OwnCloudFeed::OwnCloudFeed(RootItem* parent) : Feed(parent) {}

bool OwnCloudFeed::canBeDeleted() const {
  return true;
}

bool OwnCloudFeed::deleteViaGui() {
  if (serviceRoot()->network()->deleteFeed(customId(), getParentServiceRoot()->networkProxy()) &&
      removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

bool OwnCloudFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, this, serviceRoot()->accountId());
}

OwnCloudServiceRoot* OwnCloudFeed::serviceRoot() const {
  return qobject_cast<OwnCloudServiceRoot*>(getParentServiceRoot());
}
