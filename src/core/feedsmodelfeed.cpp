#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "core/databasefactory.h"
#include "core/feedsmodelfeed.h"


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

void FeedsModelFeed::setCountOfAllMessages(int count) {
  m_totalCount = count;
}

int FeedsModelFeed::countOfUnreadMessages() const {
  return m_unreadCount;
}

void FeedsModelFeed::setCountOfUnreadMessages(int count) {
  m_unreadCount = count;
}

FeedsModelFeed::Type FeedsModelFeed::type() const {
  return m_type;
}

void FeedsModelFeed::setType(const Type &type) {
  m_type = type;
}

void FeedsModelFeed::updateCounts() {
  //QSqlDatabase database = DatabaseFactory::getInstance()->
}
