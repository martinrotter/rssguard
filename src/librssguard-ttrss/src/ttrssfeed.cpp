// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/ttrssfeed.h"

#include "src/definitions.h"
#include "src/ttrssnetworkfactory.h"
#include "src/ttrssserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QPointer>

TtRssFeed::TtRssFeed(RootItem* parent) : Feed(parent), m_actionShareToPublished(nullptr) {}

TtRssServiceRoot* TtRssFeed::serviceRoot() const {
  return qobject_cast<TtRssServiceRoot*>(account());
}

bool TtRssFeed::canBeDeleted() const {
  return true;
}

bool TtRssFeed::deleteItem() {
  TtRssUnsubscribeFeedResponse response =
    serviceRoot()->network()->unsubscribeFeed(customNumericId(), account()->networkProxy());

  if (response.code() == QSL(UFF_OK) && removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    qWarningNN << LOGSEC_TTRSS
               << "Unsubscribing from feed failed, received JSON:" << QUOTE_W_SPACE_DOT(response.toString());
    return false;
  }
}

QList<QAction*> TtRssFeed::contextMenuFeedsList() {
  auto menu = Feed::contextMenuFeedsList();

  if (customNumericId() == TTRSS_PUBLISHED_FEED_ID) {
    if (m_actionShareToPublished == nullptr) {
      m_actionShareToPublished =
        new QAction(qApp->icons()->fromTheme(QSL("emblem-shared")), tr("Share to published"), this);

      connect(m_actionShareToPublished, &QAction::triggered, serviceRoot(), &TtRssServiceRoot::shareToPublished);
    }

    menu.append(m_actionShareToPublished);
  }

  return menu;
}

bool TtRssFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, this, serviceRoot()->accountId());
}
