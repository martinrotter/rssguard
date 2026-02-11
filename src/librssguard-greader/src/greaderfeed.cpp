// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/greaderfeed.h"

#include "src/definitions.h"
#include "src/greadernetwork.h"
#include "src/greaderserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QPointer>

GreaderFeed::GreaderFeed(RootItem* parent) : Feed(parent) {}

GreaderServiceRoot* GreaderFeed::serviceRoot() const {
  return qobject_cast<GreaderServiceRoot*>(account());
}

bool GreaderFeed::canBeDeleted() const {
  return true;
}

void GreaderFeed::deleteItem() {
  serviceRoot()->network()->subscriptionEdit(QSL(GREADER_API_EDIT_SUBSCRIPTION_DELETE),
                                             customId(),
                                             {},
                                             {},
                                             {},
                                             serviceRoot()->networkProxy());
  removeItself();
  serviceRoot()->requestItemRemoval(this, false);
}

void GreaderFeed::removeItself() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteFeed(db, this, serviceRoot()->accountId());
  });
}
