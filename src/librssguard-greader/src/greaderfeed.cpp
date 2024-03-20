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
  return qobject_cast<GreaderServiceRoot*>(getParentServiceRoot());
}

bool GreaderFeed::canBeDeleted() const {
  return true;
}

bool GreaderFeed::deleteItem() {
  try {
    serviceRoot()->network()->subscriptionEdit(QSL(GREADER_API_EDIT_SUBSCRIPTION_DELETE),
                                               customId(),
                                               {},
                                               {},
                                               {},
                                               serviceRoot()->networkProxy());
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  catch (const ApplicationException& ex) {
    qWarningNN << LOGSEC_GREADER << "Unsubscribing from feed failed, error:" << QUOTE_W_SPACE_DOT(ex.message());
    return false;
  }
}

bool GreaderFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, this, serviceRoot()->accountId());
}
