#include "core/feedsmodelfeed.h"

#include "core/databasefactory.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>


FeedsModelFeed::FeedsModelFeed(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item), m_totalCount(0), m_unreadCount(0) {
  m_kind = FeedsModelRootItem::Feed;
}

FeedsModelFeed::~FeedsModelFeed() {
}

int FeedsModelFeed::childCount() const {
  // Because feed has no children.
  return 0;
}

int FeedsModelFeed::countOfAllMessages() const {
  return m_totalCount;
}

int FeedsModelFeed::countOfUnreadMessages() const {
  return m_unreadCount;
}

QString FeedsModelFeed::typeToString(FeedsModelFeed::Type type) {
  switch (type) {
    case StandardAtom10:
      return QObject::tr("ATOM 1.0");

    case StandardRdf:
      return QObject::tr("RDF");

    case StandardRss0X:
      return QObject::tr("RSS 0.91/0.92/0.93");

    case StandardRss2X:
    default:
      return QObject::tr("RSS 2.0/2.0.1");
  }
}

void FeedsModelFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = DatabaseFactory::instance()->connection("FeedsModelFeed",
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);
  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count() FROM messages "
                               "WHERE feed = %1 AND deleted = 0;").arg(id())) &&
        query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count() FROM messages "
                             "WHERE feed = %1 AND deleted = 0 AND read = 0;").arg(id())) &&
      query_all.next()) {
    m_unreadCount = query_all.value(0).toInt();
  }
}



