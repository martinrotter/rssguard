#include <QQueue>

#include "core/feedsmodelcategory.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"


FeedsModelCategory::FeedsModelCategory(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item) {
  m_kind = FeedsModelRootItem::Category;
}

FeedsModelCategory::~FeedsModelCategory() {
}

QList<FeedsModelFeed*> FeedsModelCategory::feeds() {
  QList<FeedsModelFeed*> feeds;
  QQueue<FeedsModelCategory*> categories;

  categories.enqueue(this);

  while (!categories.isEmpty()) {
    FeedsModelCategory *active_category = categories.dequeue();

    foreach (FeedsModelRootItem *child, active_category->childItems()) {
      switch (child->kind()) {
        case FeedsModelRootItem::Feed:
          feeds.append(static_cast<FeedsModelFeed*>(child));
          break;

        case FeedsModelRootItem::Category:
          // This is category, so add it to traversed categories.
          categories.enqueue(static_cast<FeedsModelCategory*>(child));
          break;

        default:
          break;
      }
    }
  }

  return feeds;
}

int FeedsModelCategory::countOfAllMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfAllMessages();
  }

  return total_count;
}

int FeedsModelCategory::countOfUnreadMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfUnreadMessages();
  }

  return total_count;
}

FeedsModelCategory:: Type FeedsModelCategory::type() const {
  return m_type;
}

void FeedsModelCategory::setType(const Type &type) {
  m_type = type;
}

QString FeedsModelCategory::description() const {
  return m_description;
}

void FeedsModelCategory::setDescription(const QString &description) {
  m_description = description;
}

QDateTime FeedsModelCategory::creationDate() const {
  return m_creationDate;
}

void FeedsModelCategory::setCreationDate(const QDateTime &creation_date) {
  m_creationDate = creation_date;
}
