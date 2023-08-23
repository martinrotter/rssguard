// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/category.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/feed.h"
#include "services/abstract/serviceroot.h"

Category::Category(RootItem* parent) : RootItem(parent) {
  setKind(RootItem::Kind::Category);
}

Category::Category(const Category& other) : RootItem(other) {
  setKind(RootItem::Kind::Category);
}

void Category::updateCounts(bool including_total_count) {
  QList<Feed*> feeds;
  auto str = childItems();

  for (RootItem* child : qAsConst(str)) {
    if (child->kind() == RootItem::Kind::Feed) {
      feeds.append(child->toFeed());
    }
    else if (child->kind() == RootItem::Kind::Category) {
      child->updateCounts(including_total_count);
    }
  }

  if (feeds.isEmpty()) {
    return;
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  bool ok;
  auto counts = DatabaseQueries::getMessageCountsForCategory(database,
                                                             customId(),
                                                             getParentServiceRoot()->accountId(),
                                                             including_total_count,
                                                             &ok);

  if (ok) {
    for (Feed* feed : feeds) {
      if (counts.contains(feed->customId())) {
        feed->setCountOfUnreadMessages(counts.value(feed->customId()).m_unread);

        if (including_total_count) {
          feed->setCountOfAllMessages(counts.value(feed->customId()).m_total);
        }
      }
    }
  }
}

bool Category::cleanMessages(bool clean_read_only) {
  return getParentServiceRoot()->cleanFeeds(getSubTreeFeeds(), clean_read_only);
}

bool Category::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  return service->markFeedsReadUnread(getSubTreeFeeds(), status);
}
