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

void TtRssFeed::deleteItem() {
  TtRssUnsubscribeFeedResponse response =
    serviceRoot()->network()->unsubscribeFeed(customNumericId(), account()->networkProxy());

  if (response.code() == QSL(UFF_OK)) {
    removeItself();
    serviceRoot()->requestItemRemoval(this);
  }
  else {
    throw ApplicationException(response.toString());
  }
}

/*
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
*/

void TtRssFeed::removeItself() {
  DatabaseQueries::deleteFeed(qApp->database()->driver()->connection(metaObject()->className()),
                              this,
                              serviceRoot()->accountId());
}
