// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/serviceroot.h"

#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"

ServiceRoot::ServiceRoot(RootItem* parent)
  : RootItem(parent), m_recycleBin(new RecycleBin(this)), m_importantNode(new ImportantNode(this)), m_accountId(NO_PARENT_CATEGORY) {
  setKind(RootItem::Kind::ServiceRoot);
  setCreationDate(QDateTime::currentDateTime());
}

ServiceRoot::~ServiceRoot() = default;

bool ServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteAccount(database, accountId())) {
    stop();
    requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

bool ServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  auto* cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(customIDSOfMessagesForItem(this), status);
  }

  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::markAccountReadUnread(database, accountId(), status)) {
    updateCounts(false);
    itemChanged(getSubTree());
    requestReloadMessageList(status == RootItem::ReadStatus::Read);
    return true;
  }
  else {
    return false;
  }
}

QList<QAction*> ServiceRoot::addItemMenu() {
  return QList<QAction*>();
}

RecycleBin* ServiceRoot::recycleBin() const {
  return m_recycleBin;
}

bool ServiceRoot::downloadAttachmentOnMyOwn(const QUrl& url) const {
  Q_UNUSED(url)
  return false;
}

QList<QAction*> ServiceRoot::contextMenuFeedsList() {
  return serviceMenu();
}

QList<QAction*> ServiceRoot::contextMenuMessagesList(const QList<Message>& messages) {
  Q_UNUSED(messages)
  return {};
}

QList<QAction*> ServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty() && isSyncable()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Sync in"), this);
    connect(m_actionSyncIn, &QAction::triggered, this, &ServiceRoot::syncIn);
    m_serviceMenu.append(m_actionSyncIn);
  }

  return m_serviceMenu;
}

bool ServiceRoot::isSyncable() const {
  return false;
}

void ServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
}

void ServiceRoot::stop() {}

void ServiceRoot::updateCounts(bool including_total_count) {
  QList<Feed*> feeds;

  for (RootItem* child : getSubTree()) {
    if (child->kind() == RootItem::Kind::Feed) {
      feeds.append(child->toFeed());
    }
    else if (child->kind() != RootItem::Kind::Category && child->kind() != RootItem::Kind::ServiceRoot) {
      child->updateCounts(including_total_count);
    }
  }

  if (feeds.isEmpty()) {
    return;
  }

  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  bool ok;
  QMap<QString, QPair<int, int>> counts = DatabaseQueries::getMessageCountsForAccount(database, accountId(), including_total_count, &ok);

  if (ok) {
    for (Feed* feed : feeds) {
      if (counts.contains(feed->customId())) {
        feed->setCountOfUnreadMessages(counts.value(feed->customId()).first);

        if (including_total_count) {
          feed->setCountOfAllMessages(counts.value(feed->customId()).second);
        }
      }
      else {
        feed->setCountOfUnreadMessages(0);

        if (including_total_count) {
          feed->setCountOfAllMessages(0);
        }
      }
    }
  }
}

void ServiceRoot::completelyRemoveAllData() {
  // Purge old data from SQL and clean all model items.
  removeOldAccountFromDatabase(true);
  cleanAllItemsFromModel();
  updateCounts(true);
  itemChanged(QList<RootItem*>() << this);
  requestReloadMessageList(true);
}

void ServiceRoot::removeOldAccountFromDatabase(bool including_messages) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  DatabaseQueries::deleteAccountData(database, accountId(), including_messages);
}

void ServiceRoot::cleanAllItemsFromModel() {
  for (RootItem* top_level_item : childItems()) {
    if (top_level_item->kind() != RootItem::Kind::Bin && top_level_item->kind() != RootItem::Kind::Important) {
      requestItemRemoval(top_level_item);
    }
  }
}

bool ServiceRoot::cleanFeeds(QList<Feed*> items, bool clean_read_only) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::cleanFeeds(database, textualFeedIds(items), clean_read_only, accountId())) {
    // Messages are cleared, now inform model about need to reload data.
    QList<RootItem*> itemss;

    for (Feed* feed : items) {
      feed->updateCounts(true);
      itemss.append(feed);
    }

    RecycleBin* bin = recycleBin();

    if (bin != nullptr) {
      bin->updateCounts(true);
      itemss.append(bin);
    }

    ImportantNode* imp = importantNode();

    if (imp != nullptr) {
      imp->updateCounts(true);
      itemss << imp;
    }

    itemChanged(itemss);
    requestReloadMessageList(true);
    return true;
  }
  else {
    return false;
  }
}

void ServiceRoot::storeNewFeedTree(RootItem* root) {
  DatabaseQueries::storeAccountTree(qApp->database()->connection(metaObject()->className()), root, accountId());

  /*if (DatabaseQueries::storeAccountTree(database, root, accountId())) {
     RecycleBin* bin = recycleBin();

     if (bin != nullptr && !childItems().contains(bin)) {
      // As the last item, add recycle bin, which is needed.
      appendChild(bin);
      bin->updateCounts(true);
     }

     ImportantNode* imp = importantNode();

     if (imp != nullptr && !childItems().contains(imp)) {
      appendChild(imp);
      imp->updateCounts(true);
     }
     }*/
}

void ServiceRoot::removeLeftOverMessages() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  DatabaseQueries::purgeLeftoverMessages(database, accountId());
}

void ServiceRoot::removeLeftOverMessageFilterAssignments() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  DatabaseQueries::purgeLeftoverMessageFilterAssignments(database, accountId());
}

QList<Message> ServiceRoot::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesForAccount(database, accountId());
}

bool ServiceRoot::supportsFeedAdding() const {
  return false;
}

bool ServiceRoot::supportsCategoryAdding() const {
  return false;
}

void ServiceRoot::itemChanged(const QList<RootItem*>& items) {
  emit dataChanged(items);
}

void ServiceRoot::requestReloadMessageList(bool mark_selected_messages_read) {
  emit reloadMessageListRequested(mark_selected_messages_read);
}

void ServiceRoot::requestItemExpand(const QList<RootItem*>& items, bool expand) {
  emit itemExpandRequested(items, expand);
}

void ServiceRoot::requestItemExpandStateSave(RootItem* subtree_root) {
  emit itemExpandStateSaveRequested(subtree_root);
}

void ServiceRoot::requestItemReassignment(RootItem* item, RootItem* new_parent) {
  emit itemReassignmentRequested(item, new_parent);
}

void ServiceRoot::requestItemRemoval(RootItem* item) {
  emit itemRemovalRequested(item);
}

void ServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
  Q_UNUSED(selected_item)
  Q_UNUSED(url)
}

void ServiceRoot::addNewCategory(RootItem* selected_item) {
  Q_UNUSED(selected_item)
}

QMap<QString, QVariantMap> ServiceRoot::storeCustomFeedsData() {
  QMap<QString, QVariantMap> custom_data;

  for (const Feed* feed : getSubTreeFeeds()) {
    QVariantMap feed_custom_data;

    feed_custom_data.insert(QSL("auto_update_interval"), feed->autoUpdateInitialInterval());
    feed_custom_data.insert(QSL("auto_update_type"), int(feed->autoUpdateType()));
    feed_custom_data.insert(QSL("msg_filters"), QVariant::fromValue(feed->messageFilters()));
    custom_data.insert(feed->customId(), feed_custom_data);
  }

  return custom_data;
}

void ServiceRoot::restoreCustomFeedsData(const QMap<QString, QVariantMap>& data, const QHash<QString, Feed*>& feeds) {
  QMapIterator<QString, QVariantMap> i(data);

  while (i.hasNext()) {
    i.next();
    const QString custom_id = i.key();

    if (feeds.contains(custom_id)) {
      Feed* feed = feeds.value(custom_id);
      QVariantMap feed_custom_data = i.value();

      feed->setAutoUpdateInitialInterval(feed_custom_data.value(QSL("auto_update_interval")).toInt());
      feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(feed_custom_data.value(QSL("auto_update_type")).toInt()));
      feed->setMessageFilters(feed_custom_data.value(QSL("msg_filters")).value<QList<QPointer<MessageFilter>>>());
    }
  }
}

ImportantNode* ServiceRoot::importantNode() const {
  return m_importantNode;
}

void ServiceRoot::setRecycleBin(RecycleBin* recycle_bin) {
  m_recycleBin = recycle_bin;
}

void ServiceRoot::syncIn() {
  QIcon original_icon = icon();

  setIcon(qApp->icons()->fromTheme(QSL("view-refresh")));
  itemChanged(QList<RootItem*>() << this);
  RootItem* new_tree = obtainNewTreeForSyncIn();

  if (new_tree != nullptr) {
    auto feed_custom_data = storeCustomFeedsData();

    // Remove from feeds model, then from SQL but leave messages intact.
    cleanAllItemsFromModel();
    removeOldAccountFromDatabase(false);
    restoreCustomFeedsData(feed_custom_data, new_tree->getHashedSubTreeFeeds());

    // Model is clean, now store new tree into DB and
    // set primary IDs of the items.
    storeNewFeedTree(new_tree);

    // We have new feed, some feeds were maybe removed,
    // so remove left over messages and filter assignments.
    removeLeftOverMessages();
    removeLeftOverMessageFilterAssignments();

    for (RootItem* top_level_item : new_tree->childItems()) {
      top_level_item->setParent(nullptr);
      requestItemReassignment(top_level_item, this);
    }

    new_tree->clearChildren();
    new_tree->deleteLater();

    updateCounts(true);
    requestReloadMessageList(true);
  }

  setIcon(original_icon);
  itemChanged(getSubTree());
}

RootItem* ServiceRoot::obtainNewTreeForSyncIn() const {
  return nullptr;
}

QStringList ServiceRoot::customIDSOfMessagesForItem(RootItem* item) {
  if (item->getParentServiceRoot() != this) {
    // Not item from this account.
    return QStringList();
  }
  else {
    QStringList list;

    switch (item->kind()) {
      case RootItem::Kind::Category: {
        for (RootItem* child : item->childItems()) {
          list.append(customIDSOfMessagesForItem(child));
        }

        return list;
      }

      case RootItem::Kind::ServiceRoot: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromAccount(database, accountId());
        break;
      }

      case RootItem::Kind::Bin: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromBin(database, accountId());
        break;
      }

      case RootItem::Kind::Feed: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromFeed(database, item->customId(), accountId());
        break;
      }

      case RootItem::Kind::Important: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfImportantMessages(database, accountId());
        break;
      }

      default:
        break;
    }

    qDebug() << "Custom IDs of messages for some operation are:" << list;
    return list;
  }
}

bool ServiceRoot::markFeedsReadUnread(QList<Feed*> items, RootItem::ReadStatus read) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::markFeedsReadUnread(database, textualFeedIds(items), accountId(), read)) {
    QList<RootItem*> itemss;

    for (Feed* feed : items) {
      feed->updateCounts(false);
      itemss.append(feed);
    }

    ImportantNode* imp = importantNode();

    if (imp != nullptr) {
      imp->updateCounts(true);
      itemss << imp;
    }

    itemChanged(itemss);
    requestReloadMessageList(read == RootItem::ReadStatus::Read);
    return true;
  }
  else {
    return false;
  }
}

QStringList ServiceRoot::textualFeedUrls(const QList<Feed*>& feeds) const {
  QStringList stringy_urls;

  stringy_urls.reserve(feeds.size());

  for (const Feed* feed : feeds) {
    stringy_urls.append(!feed->url().isEmpty() ? feed->url() : QL1S("no-url"));
  }

  return stringy_urls;
}

QStringList ServiceRoot::textualFeedIds(const QList<Feed*>& feeds) const {
  QStringList stringy_ids;

  stringy_ids.reserve(feeds.size());

  for (const Feed* feed : feeds) {
    stringy_ids.append(QString("'%1'").arg(feed->customId()));
  }

  return stringy_ids;
}

QStringList ServiceRoot::customIDsOfMessages(const QList<ImportanceChange>& changes) {
  QStringList list;

  for (const auto& change : changes) {
    list.append(change.first.m_customId);
  }

  return list;
}

QStringList ServiceRoot::customIDsOfMessages(const QList<Message>& messages) {
  QStringList list;

  for (const Message& message : messages) {
    list.append(message.m_customId);
  }

  return list;
}

int ServiceRoot::accountId() const {
  return m_accountId;
}

void ServiceRoot::setAccountId(int account_id) {
  m_accountId = account_id;
}

bool ServiceRoot::loadMessagesForItem(RootItem* item, MessagesModel* model) {
  if (item->kind() == RootItem::Kind::Bin) {
    model->setFilter(QString("Messages.is_deleted = 1 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1")
                     .arg(QString::number(accountId())));
  }
  else if (item->kind() == RootItem::Kind::Important) {
    model->setFilter(QString("Messages.is_important = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1")
                     .arg(QString::number(accountId())));
  }
  else if (item->kind() == RootItem::Kind::Label) {
    // Show messages with particular label.
    model->setFilter(QString("Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1 AND "
                             "(SELECT COUNT(*) FROM LabelsInMessages WHERE account_id = %1 AND message = Messages.custom_id AND label = %2) > 0")
                     .arg(QString::number(accountId()), QString::number(item->id())));
  }
  else if (item->kind() == RootItem::Kind::Labels) {
    // Show messages with any label.
    model->setFilter(QString("Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1 AND "
                             "(SELECT COUNT(*) FROM LabelsInMessages WHERE account_id = %1 AND message = Messages.custom_id) > 0")
                     .arg(QString::number(accountId())));
  }
  else {
    QList<Feed*> children = item->getSubTreeFeeds();
    QString filter_clause = textualFeedIds(children).join(QSL(", "));

    if (filter_clause.isEmpty()) {
      filter_clause = QSL("null");
    }

    model->setFilter(
      QString("Feeds.custom_id IN (%1) AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = %2").arg(
        filter_clause,
        QString::
        number(accountId())));

    QString urls = textualFeedUrls(children).join(QSL(", "));

    qDebug("Displaying messages from feeds IDs: %s and URLs: %s.", qPrintable(filter_clause), qPrintable(urls));
  }

  return true;
}

bool ServiceRoot::onBeforeSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(customIDsOfMessages(messages), read);
  }

  return true;
}

bool ServiceRoot::onAfterSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, RootItem::ReadStatus read) {
  Q_UNUSED(messages)
  Q_UNUSED(read)

  QList<RootItem*> items_to_reload;
  ImportantNode* imp = importantNode();

  if (imp == selected_item) {
    updateCounts(true);
    items_to_reload << getSubTree();
  }
  else {
    selected_item->updateCounts(false);
    items_to_reload << selected_item;
  }

  if (imp != nullptr && imp != selected_item) {
    imp->updateCounts(true);
    items_to_reload << imp;
  }

  itemChanged(items_to_reload);
  return true;
}

bool ServiceRoot::onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes) {
  Q_UNUSED(selected_item)

  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    // Now, we need to separate the changes because of Nextcloud API limitations.
    QList<Message> mark_starred_msgs;
    QList<Message> mark_unstarred_msgs;

    for (const ImportanceChange& pair : changes) {
      if (pair.second == RootItem::Importance::Important) {
        mark_starred_msgs.append(pair.first);
      }
      else {
        mark_unstarred_msgs.append(pair.first);
      }
    }

    if (!mark_starred_msgs.isEmpty()) {
      cache->addMessageStatesToCache(mark_starred_msgs, RootItem::Importance::Important);
    }

    if (!mark_unstarred_msgs.isEmpty()) {
      cache->addMessageStatesToCache(mark_unstarred_msgs, RootItem::Importance::NotImportant);
    }
  }

  return true;
}

bool ServiceRoot::onAfterSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes) {
  Q_UNUSED(selected_item)
  Q_UNUSED(changes)

  QList<RootItem*> items_to_reload;
  ImportantNode* imp = importantNode();

  if (imp != nullptr) {
    imp->updateCounts(true);
    items_to_reload << imp;
  }

  itemChanged(items_to_reload);
  return true;
}

bool ServiceRoot::onBeforeMessagesDelete(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)
  return true;
}

bool ServiceRoot::onAfterMessagesDelete(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(messages)

  QList<RootItem*> items_to_reload;

  selected_item->updateCounts(true);
  items_to_reload << selected_item;

  RecycleBin* bin = recycleBin();

  if (bin != nullptr && bin != selected_item) {
    bin->updateCounts(true);
    items_to_reload << bin;
  }

  ImportantNode* imp = importantNode();

  if (imp != nullptr && imp != selected_item) {
    imp->updateCounts(true);
    items_to_reload << imp;
  }

  itemChanged(items_to_reload);
  return true;
}

bool ServiceRoot::onBeforeMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)
  return true;
}

bool ServiceRoot::onAfterMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)
  updateCounts(true);
  itemChanged(getSubTree());
  return true;
}

void ServiceRoot::assembleFeeds(Assignment feeds) {
  QHash<int, Category*> categories = getHashedSubTreeCategories();

  for (const AssignmentItem& feed : feeds) {
    if (feed.first == NO_PARENT_CATEGORY) {
      // This is top-level feed, add it to the root item.
      appendChild(feed.second);
    }
    else if (categories.contains(feed.first)) {
      // This feed belongs to this category.
      categories.value(feed.first)->appendChild(feed.second);
    }
    else {
      qWarning("Feed '%s' is loose, skipping it.", qPrintable(feed.second->title()));
    }
  }
}

void ServiceRoot::assembleCategories(Assignment categories) {
  QHash<int, RootItem*> assignments;

  assignments.insert(NO_PARENT_CATEGORY, this);

  // Add top-level categories.
  while (!categories.isEmpty()) {
    for (int i = 0; i < categories.size(); i++) {
      if (assignments.contains(categories.at(i).first)) {
        // Parent category of this category is already added.
        assignments.value(categories.at(i).first)->appendChild(categories.at(i).second);

        // Now, added category can be parent for another categories, add it.
        assignments.insert(categories.at(i).second->id(), categories.at(i).second);

        // Remove the category from the list, because it was
        // added to the final collection.
        categories.removeAt(i);
        i--;
      }
    }
  }
}
