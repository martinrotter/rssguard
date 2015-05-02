#include "core/feedsselection.h"

#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelfeed.h"


FeedsSelection::FeedsSelection(FeedsModelRootItem *root_of_selection) : m_selectedItem(root_of_selection) {
}

FeedsSelection::FeedsSelection(const FeedsSelection &other) {
  m_selectedItem = other.selectedItem();
}

FeedsSelection::~FeedsSelection() {
}

FeedsSelection::MessageMode FeedsSelection::mode() {
  if (m_selectedItem == NULL) {
    return MessageMode::NoMode;
  }

  switch (m_selectedItem->kind()) {
    case FeedsModelRootItem::RecycleBin:
      return MessageMode::MessagesFromRecycleBin;

    case FeedsModelRootItem::Category:
    case FeedsModelRootItem::Feed:
      return MessageMode::MessagesFromFeeds;

    default:
      return MessageMode::NoMode;
  }
}

FeedsModelRootItem *FeedsSelection::selectedItem() const {
  return m_selectedItem;
}

QString FeedsSelection::generateListOfIds() {
  if (m_selectedItem != NULL &&
      (m_selectedItem->kind() == FeedsModelRootItem::Feed || m_selectedItem->kind() == FeedsModelRootItem::Category)) {
    QList<FeedsModelRootItem*> children = m_selectedItem->getRecursiveChildren();
    QStringList stringy_ids;

    children.append(m_selectedItem);

    foreach (FeedsModelRootItem *child, children) {
      if (child->kind() == FeedsModelRootItem::Feed) {
        stringy_ids.append(QString::number(child->id()));
      }
    }

    return stringy_ids.join(", ");
  }
  else {
    return QString();
  }
}
