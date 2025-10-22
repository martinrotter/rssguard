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

  for (RootItem* child : std::as_const(str)) {
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
                                                             account()->accountId(),
                                                             including_total_count,
                                                             &ok);

  if (ok) {
    for (Feed* feed : feeds) {
      if (counts.contains(feed->id())) {
        feed->setCountOfUnreadMessages(counts.value(feed->id()).m_unread);

        if (including_total_count) {
          feed->setCountOfAllMessages(counts.value(feed->id()).m_total);
        }
      }
    }
  }
}

void Category::cleanMessages(bool clean_read_only) {
  account()->cleanFeeds(getSubTreeFeeds(), clean_read_only);
}

void Category::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  service->markFeedsReadUnread(getSubTreeFeeds(), status);
}

QString Category::additionalTooltip() const {
  return tr("Number of feeds: %1\n"
            "Number of categories: %2\n"
            "Number of disabled feeds: %3")
    .arg(QString::number(getSubTreeFeeds().size()),
         QString::number(getSubTreeCategories().size() - 1),
         QString::number(getSubTree<RootItem>([](const RootItem* ri) {
                           return ri->kind() == RootItem::Kind::Feed && ri->toFeed()->isSwitchedOff();
                         }).size()));
}
