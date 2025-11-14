// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/rootitem.h"

#include "3rd-party/boolinq/boolinq.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/label.h"
#include "services/abstract/search.h"
#include "services/abstract/serviceroot.h"

#include <QVariant>

RootItem::RootItem(RootItem* parent_item)
  : QObject(nullptr), m_kind(RootItem::Kind::Root), m_id(NO_PARENT_CATEGORY), m_customId(QString()), m_title(QString()),
    m_description(QString()), m_creationDate(QDateTime::currentDateTimeUtc()), m_keepOnTop(false),
    m_sortOrder(NO_PARENT_CATEGORY), m_childItems(QList<RootItem*>()), m_parentItem(parent_item), m_deleting(false) {}

RootItem::RootItem(const RootItem& other) : RootItem(nullptr) {
  setTitle(other.title());
  setId(other.id());
  setCustomId(other.customId());
  setIcon(other.icon());
  setKeepOnTop(other.keepOnTop());
  setSortOrder(other.sortOrder());

  // NOTE: We do not need to clone childs, because that would mean that
  // either source or target item tree would get corrupted.
  // setChildItems(other.childItems());

  setParent(other.parent());
  setCreationDate(other.creationDate());
  setDescription(other.description());
  setDeleting(other.deleting());
}

RootItem::~RootItem() {
  qDeleteAll(m_childItems);
}

QString RootItem::hashCode() const {
  ServiceRoot* root = account();
  int acc_id = root == nullptr ? 0 : root->accountId();

  return QString::number(acc_id) + QL1S("-") + QString::number(int(kind())) + QL1S("-") + QString::number(id());
}

QString RootItem::additionalTooltip() const {
  return QString();
}

bool RootItem::canBeEdited() const {
  return false;
}

bool RootItem::canBeDeleted() const {
  return false;
}

void RootItem::deleteItem() {}

bool RootItem::isFetching() const {
  return false;
}

void RootItem::markAsReadUnread(ReadStatus status) {
  for (RootItem* child : std::as_const(m_childItems)) {
    child->markAsReadUnread(status);
  }
}

void RootItem::cleanMessages(bool clear_only_read) {
  for (RootItem* child : std::as_const(m_childItems)) {
    if (child->kind() != RootItem::Kind::Bin) {
      child->cleanMessages(clear_only_read);
    }
  }
}

void RootItem::updateCounts() {
  for (RootItem* child : std::as_const(m_childItems)) {
    child->updateCounts();
  }
}

int RootItem::row() const {
  if (m_parentItem != nullptr) {
    return m_parentItem->m_childItems.indexOf(const_cast<RootItem*>(this));
  }
  else {
    // This item has no parent. Therefore, its row index is 0.
    return 0;
  }
}

QVariant RootItem::data(int column, int role) const {
  switch (role) {
    case Qt::ItemDataRole::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        QString tool_tip = QSL("<b>%1</b>").arg(m_title);

        if (!m_description.isEmpty()) {
          tool_tip += QL1S("\n") + m_description;
        }

        QString extra_tooltip = additionalTooltip();

        if (!extra_tooltip.isEmpty()) {
          tool_tip += QL1S("\n\n") + extra_tooltip;
        }

        return tool_tip.replace(QSL("\n"), QSL("<br />"));
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread article(s).", nullptr, countOfUnreadMessages());
      }
      else {
        return QVariant();
      }

    case LOWER_TITLE_ROLE:
      return m_title.toLower();

    case Qt::ItemDataRole::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_ID_INDEX) {
        return id();
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::ItemDataRole::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return sanitizedTitle();
      }
      else if (column == FDS_MODEL_ID_INDEX) {
        return id() < 0 ? QVariant() : id();
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        int count_unread = countOfUnreadMessages();

        if (count_unread <= 0 && qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::HideCountsIfNoUnread)).toBool()) {
          return QString();
        }
        else {
          int count_all = countOfAllMessages();

          return qApp->settings()
            ->value(GROUP(Feeds), SETTING(Feeds::CountFormat))
            .toString()
            .replace(QSL(PLACEHOLDER_UNREAD_COUNTS), count_unread < 0 ? QSL("-") : QString::number(count_unread))
            .replace(QSL(PLACEHOLDER_ALL_COUNTS), count_all < 0 ? QSL("-") : QString::number(count_all));
        }
      }
      else {
        return QVariant();
      }

    case Qt::ItemDataRole::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return fullIcon();
      }
      else {
        return QVariant();
      }

    case Qt::ItemDataRole::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignmentFlag::AlignCenter;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}

Qt::ItemFlags RootItem::additionalFlags() const {
  if (m_kind == RootItem::Kind::Root) {
    return Qt::ItemFlag::ItemIsDropEnabled;
  }
  else {
    return Qt::ItemFlag::NoItemFlags;
  }
}

bool RootItem::performDragDropChange(RootItem* target_item) {
  Q_UNUSED(target_item)
  return false;
}

int RootItem::countOfUnreadMessages() const {
  return boolinq::from(m_childItems).sum([](RootItem* it) {
    return (it->kind() == RootItem::Kind::Bin || it->kind() == RootItem::Kind::Important ||
            it->kind() == RootItem::Kind::Unread || it->kind() == RootItem::Kind::Labels ||
            it->kind() == RootItem::Kind::Probes)
             ? 0
             : std::max(it->countOfUnreadMessages(), 0);
  });
}

int RootItem::countOfAllMessages() const {
  return boolinq::from(m_childItems).sum([](RootItem* it) {
    return (it->kind() == RootItem::Kind::Bin || it->kind() == RootItem::Kind::Important ||
            it->kind() == RootItem::Kind::Unread || it->kind() == RootItem::Kind::Labels ||
            it->kind() == RootItem::Kind::Probes)
             ? 0
             : std::max(it->countOfAllMessages(), 0);
  });
}

bool RootItem::isChildOf(const RootItem* root) const {
  if (root == nullptr) {
    return false;
  }

  const RootItem* this_item = this;

  while (this_item->kind() != RootItem::Kind::Root) {
    if (root->childItems().contains(const_cast<RootItem* const>(this_item))) {
      return true;
    }
    else {
      this_item = this_item->parent();
    }
  }

  return false;
}

bool RootItem::isParentOf(const RootItem* child) const {
  if (child == nullptr) {
    return false;
  }
  else {
    return child->isChildOf(this);
  }
}

QList<RootItem*> RootItem::getSubTree(RootItem::Kind kind_of_item) const {
  return getSubTree<RootItem>([kind_of_item](const RootItem* ri) {
    return int(ri->kind() & kind_of_item) > 0;
  });
}

QList<Category*> RootItem::getSubTreeCategories() const {
  return getSubTree<Category>([](const RootItem* ri) {
    return ri->kind() == RootItem::Kind::Category;
  });
}

RootItem* RootItem::getItemFromSubTree(std::function<bool(const RootItem*)> tester) const {
  QList<RootItem*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (tester(active_item)) {
      return active_item;
    }

    children.append(active_item);
    traversable_items.append(active_item->childItems());
  }

  return nullptr;
}

QHash<int, Category*> RootItem::getSubTreeCategoriesForAssemble() const {
  QHash<int, Category*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Category && !children.contains(active_item->id())) {
      children.insert(active_item->id(), active_item->toCategory());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QHash<QString, Category*> RootItem::getHashedSubTreeCategories() const {
  QHash<QString, Category*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Category && !children.contains(active_item->customId())) {
      children.insert(active_item->customId(), active_item->toCategory());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QHash<QString, Feed*> RootItem::getHashedSubTreeFeeds() const {
  QHash<QString, Feed*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Feed && !children.contains(active_item->customId())) {
      children.insert(active_item->customId(), active_item->toFeed());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QHash<int, Feed*> RootItem::getPrimaryIdHashedSubTreeFeeds() const {
  QHash<int, Feed*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Feed && !children.contains(active_item->id())) {
      children.insert(active_item->id(), active_item->toFeed());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QList<Feed*> RootItem::getSubTreeFeeds(bool recursive) const {
  QList<Feed*> children;
  QList<RootItem*> traversable_items;
  bool traversed = false;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Feed) {
      children.append(active_item->toFeed());
    }

    if (recursive || !traversed) {
      traversed = true;
      traversable_items.append(active_item->childItems());
    }
  }

  return children;
}

QList<Feed*> RootItem::getSubTreeAutoFetchingWithManualIntervalsFeeds() const {
  QList<Feed*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Feed &&
        active_item->toFeed()->autoUpdateType() == Feed::AutoUpdateType::SpecificAutoUpdate) {
      children.append(active_item->toFeed());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

QList<Feed*> RootItem::getSubAutoFetchingEnabledFeeds() const {
  QList<Feed*> children;
  QList<RootItem*> traversable_items;

  traversable_items.append(const_cast<RootItem* const>(this));

  // Iterate all nested items.
  while (!traversable_items.isEmpty()) {
    RootItem* active_item = traversable_items.takeFirst();

    if (active_item->kind() == RootItem::Kind::Feed &&
        active_item->toFeed()->autoUpdateType() != Feed::AutoUpdateType::DontAutoUpdate) {
      children.append(active_item->toFeed());
    }

    traversable_items.append(active_item->childItems());
  }

  return children;
}

ServiceRoot* RootItem::account() const {
  const RootItem* working_parent = this;

  while (working_parent->kind() != RootItem::Kind::Root) {
    if (working_parent->kind() == RootItem::Kind::ServiceRoot) {
      return working_parent->toServiceRoot();
    }
    else {
      working_parent = working_parent->parent();
    }
  }

  return nullptr;
}

RootItem::Kind RootItem::kind() const {
  return m_kind;
}

void RootItem::setKind(RootItem::Kind kind) {
  m_kind = kind;
}

QIcon RootItem::icon() const {
  return m_icon;
}

void RootItem::setIcon(const QIcon& icon) {
  m_icon = icon;
}

QIcon RootItem::fullIcon() const {
  QIcon ico = icon();

  if (ico.isNull()) {
    if (kind() == RootItem::Kind::Feed) {
      return qApp->icons()->fromTheme(QSL("application-rss+xml"));
    }
    else if (kind() == RootItem::Kind::Category) {
      return qApp->icons()->fromTheme(QSL("folder"));
    }
  }

  return ico;
}

int RootItem::id() const {
  return m_id;
}

void RootItem::setId(int id) {
  m_id = id;
}

QString RootItem::title() const {
  return m_title;
}

QString RootItem::sanitizedTitle() const {
  return m_title.simplified();
}

void RootItem::setTitle(const QString& title) {
  m_title = title;
}

QDateTime RootItem::creationDate() const {
  return m_creationDate;
}

void RootItem::setCreationDate(const QDateTime& creation_date) {
  m_creationDate = creation_date;
}

QString RootItem::description() const {
  return m_description;
}

void RootItem::setDescription(const QString& description) {
  m_description = description;
}

bool RootItem::removeChild(RootItem* child) {
  return m_childItems.removeOne(child);
}

QString RootItem::customId() const {
  return m_customId;
}

int RootItem::customNumericId() const {
  return customId().toInt();
}

void RootItem::setCustomId(const QString& custom_id) {
  m_customId = custom_id;
}

Category* RootItem::toCategory() const {
  return qobject_cast<Category*>(const_cast<RootItem*>(this));
}

Feed* RootItem::toFeed() const {
  return qobject_cast<Feed*>(const_cast<RootItem*>(this));
}

Label* RootItem::toLabel() const {
  return qobject_cast<Label*>(const_cast<RootItem*>(this));
}

Search* RootItem::toProbe() const {
  return qobject_cast<Search*>(const_cast<RootItem*>(this));
}

ServiceRoot* RootItem::toServiceRoot() const {
  return qobject_cast<ServiceRoot*>(const_cast<RootItem*>(this));
}

bool RootItem::keepOnTop() const {
  return m_keepOnTop;
}

void RootItem::setKeepOnTop(bool keep_on_top) {
  m_keepOnTop = keep_on_top;
}

int RootItem::sortOrder() const {
  return m_sortOrder;
}

void RootItem::setSortOrder(int sort_order) {
  m_sortOrder = sort_order;
}

bool RootItem::deleting() const {
  return m_deleting;
}

void RootItem::setDeleting(bool deleting) {
  m_deleting = deleting;
}

bool RootItem::isAboutToBeDeleted() const {
  const RootItem* check = this;

  while (check != nullptr && check->kind() != RootItem::Kind::Root) {
    if (check->deleting()) {
      return true;
    }

    check = check->parent();
  }

  return false;
}

bool RootItem::removeChild(int index) {
  if (index >= 0 && index < m_childItems.size()) {
    m_childItems.removeAt(index);
    return true;
  }
  else {
    return false;
  }
}

QDataStream& operator>>(QDataStream& in, RootItem::ReadStatus& myObj) {
  int obj;

  in >> obj;

  myObj = (RootItem::ReadStatus)obj;

  return in;
}

QDataStream& operator<<(QDataStream& out, const RootItem::ReadStatus& myObj) {
  out << (int)myObj;

  return out;
}

QDataStream& operator>>(QDataStream& in, RootItem::Importance& myObj) {
  int obj;

  in >> obj;

  myObj = (RootItem::Importance)obj;

  return in;
}

QDataStream& operator<<(QDataStream& out, const RootItem::Importance& myObj) {
  out << (int)myObj;

  return out;
}

RootItem::Kind operator|(RootItem::Kind a, RootItem::Kind b) {
  return static_cast<RootItem::Kind>(static_cast<int>(a) | static_cast<int>(b));
}

RootItem::Kind operator&(RootItem::Kind a, RootItem::Kind b) {
  return static_cast<RootItem::Kind>(static_cast<int>(a) & static_cast<int>(b));
}

QList<RootItem*> RootItem::childItems(Kind kind) const {
  auto linq = boolinq::from(m_childItems)
                .where([=](RootItem* it) {
                  return it->kind() == kind;
                })
                .toStdList();

  return FROM_STD_LIST(QList<RootItem*>, linq);
}
