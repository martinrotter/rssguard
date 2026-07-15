// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/category.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "services/abstract/feed.h"
#include "services/abstract/serviceroot.h"

Category::Category(RootItem* parent) : RootItem(parent) {
  setKind(RootItem::Kind::Category);
}

Category::Category(const Category& other) : RootItem(other) {
  setKind(RootItem::Kind::Category);
}

QVariant Category::data(int column, int role) const {
  switch (role) {
    case Qt::ItemDataRole::BackgroundRole: {
      auto clr = qApp->skins()->colorForModel(SkinEnums::PaletteColors::BgFolder).value<QColor>();

      if (clr.isValid()) {
        return clr;
      }
      else {
        return RootItem::data(column, role);
      }
    }

    case HIGHLIGHTED_BACKGROUND_ROLE: {
      auto clr = qApp->skins()->colorForModel(SkinEnums::PaletteColors::BgSelectedFolder).value<QColor>();

      if (clr.isValid()) {
        return clr;
      }
      else {
        return RootItem::data(column, role);
      }
    }

    default:
      return RootItem::data(column, role);
  }
}

QVariantHash Category::customDatabaseData() const {
  return {};
}

void Category::setCustomDatabaseData(const QVariantHash& data) {
  Q_UNUSED(data)
}

void Category::updateCounts() {
  QList<Feed*> feeds = getSubTreeFeeds();

  if (feeds.isEmpty()) {
    return;
  }

  auto counts = qApp->database()->worker()->read<QMap<int, ArticleCounts>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getMessageCountsForFeeds(db, account()->textualFeedIds(feeds));
  });

  for (Feed* feed : feeds) {
    if (counts.contains(feed->id())) {
      feed->setCountOfUnreadMessages(counts.value(feed->id()).m_unread);
      feed->setCountOfAllMessages(counts.value(feed->id()).m_total);
    }
  }
}

void Category::cleanMessages(bool clean_read_only) {
  account()->cleanFeeds(getSubTreeFeeds(), clean_read_only);
}

void Category::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  executeMessagesReadUnreadChange(status, [service, this, status](const QSqlDatabase& db) {
    DatabaseQueries::markFeedsReadUnread(db, service->textualFeedIds(getSubTreeFeeds()), status);
  });
}

QString Category::additionalTooltip() const {
  return tr("Number of feeds: %1\n"
            "Number of folders: %2\n"
            "Number of disabled feeds: %3")
    .arg(QString::number(getSubTreeFeeds().size()),
         QString::number(getSubTreeCategories().size() - 1),
         QString::number(getSubTree<RootItem>([](const RootItem* ri) {
                           return ri->kind() == RootItem::Kind::Feed && ri->toFeed()->isSwitchedOff();
                         }).size()));
}
