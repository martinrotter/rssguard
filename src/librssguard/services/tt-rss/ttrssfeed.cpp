// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "database/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"

#include <QPointer>

TtRssFeed::TtRssFeed(RootItem* parent) : Feed(parent) {}

TtRssServiceRoot* TtRssFeed::serviceRoot() const {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

bool TtRssFeed::canBeDeleted() const {
  return true;
}

bool TtRssFeed::deleteViaGui() {
  TtRssUnsubscribeFeedResponse response = serviceRoot()->network()->unsubscribeFeed(customNumericId(),
                                                                                    getParentServiceRoot()->networkProxy());

  if (response.code() == UFF_OK && removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    qWarningNN << LOGSEC_TTRSS
               << "Unsubscribing from feed failed, received JSON:"
               << QUOTE_W_SPACE_DOT(response.toString());
    return false;
  }
}

bool TtRssFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, customId().toInt(), serviceRoot()->accountId());
}
