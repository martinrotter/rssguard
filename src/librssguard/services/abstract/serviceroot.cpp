// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/serviceroot.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/messagesmodel.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/gui/custommessagepreviewer.h"
#include "services/abstract/gui/formcategorydetails.h"
#include "services/abstract/gui/formfeeddetails.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/search.h"
#include "services/abstract/searchsnode.h"
#include "services/abstract/unreadnode.h"

ServiceRoot::ServiceRoot(RootItem* parent)
  : RootItem(parent), m_recycleBin(new RecycleBin(this)), m_importantNode(new ImportantNode(this)),
    m_labelsNode(new LabelsNode(this)), m_probesNode(new SearchsNode(this)), m_unreadNode(new UnreadNode(this)),
    m_accountId(NO_PARENT_CATEGORY), m_networkProxy(QNetworkProxy()) {
  setKind(RootItem::Kind::ServiceRoot);
  appendCommonNodes();
}

ServiceRoot::~ServiceRoot() {}

bool ServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::deleteAccount(database, this)) {
    stop();
    requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

void ServiceRoot::editItemsViaGui(const QList<RootItem*>& items) {
  auto std_feeds = boolinq::from(items)
                     .select([](RootItem* it) {
                       return qobject_cast<Feed*>(it);
                     })
                     .where([](Feed* fd) {
                       return fd != nullptr;
                     })
                     .toStdList();

  if (!std_feeds.empty()) {
    QScopedPointer<FormFeedDetails> form_pointer(new FormFeedDetails(this, qApp->mainFormWidget()));

    form_pointer->addEditFeed<Feed>(FROM_STD_LIST(QList<Feed*>, std_feeds));
  }
  else {
    auto std_categories = boolinq::from(items)
                            .select([](RootItem* it) {
                              return qobject_cast<Category*>(it);
                            })
                            .where([](Category* fd) {
                              return fd != nullptr;
                            })
                            .toStdList();

    if (!std_categories.empty()) {
      QScopedPointer<FormCategoryDetails> form_pointer(new FormCategoryDetails(this, nullptr, qApp->mainFormWidget()));

      // TODO: todo
      // form_pointer->addEditCategory<Feed>(FROM_STD_LIST(QList<Feed*>, std_feeds));
    }
  }
}

bool ServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  auto* cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(customIDSOfMessagesForItem(this, status), status);
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

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

QList<QAction*> ServiceRoot::contextMenuFeedsList() {
  auto specific = serviceMenu();
  auto base = RootItem::contextMenuFeedsList();

  if (!specific.isEmpty()) {
    auto* act_sep = new QAction(this);

    act_sep->setSeparator(true);
    base.append(act_sep);
    base.append(specific);
  }

  return base;
}

QList<QAction*> ServiceRoot::contextMenuMessagesList(const QList<Message>& messages) {
  Q_UNUSED(messages)
  return {};
}

QList<QAction*> ServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    if (isSyncable()) {
      auto* act_sync_tree =
        new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Synchronize folders && other items"), this);

      connect(act_sync_tree, &QAction::triggered, this, &ServiceRoot::syncIn);
      m_serviceMenu.append(act_sync_tree);

      auto* cache = toCache();

      if (cache != nullptr) {
        auto* act_sync_cache =
          new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Synchronize article cache"), this);

        connect(act_sync_cache, &QAction::triggered, this, [cache]() {
          cache->saveAllCachedData(false);
        });

        m_serviceMenu.append(act_sync_cache);
      }
    }
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

CustomMessagePreviewer* ServiceRoot::customMessagePreviewer() {
  return nullptr;
}

void ServiceRoot::updateCounts(bool including_total_count) {
  QList<Feed*> feeds;
  auto str = getSubTree();

  for (RootItem* child : qAsConst(str)) {
    if (child->kind() == RootItem::Kind::Feed) {
      feeds.append(child->toFeed());
    }
    else if (child->kind() != RootItem::Kind::Label && child->kind() != RootItem::Kind::Category &&
             child->kind() != RootItem::Kind::ServiceRoot && child->kind() != RootItem::Kind::Probe) {
      child->updateCounts(including_total_count);
    }
  }

  if (feeds.isEmpty()) {
    return;
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  bool ok;
  QMap<QString, ArticleCounts> counts =
    DatabaseQueries::getMessageCountsForAccount(database, accountId(), including_total_count, &ok);

  if (ok) {
    for (Feed* feed : feeds) {
      if (counts.contains(feed->customId())) {
        feed->setCountOfUnreadMessages(counts.value(feed->customId()).m_unread);

        if (including_total_count) {
          feed->setCountOfAllMessages(counts.value(feed->customId()).m_total);
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

bool ServiceRoot::canBeDeleted() const {
  return true;
}

void ServiceRoot::completelyRemoveAllData() {
  // Purge old data from SQL and clean all model items.
  cleanAllItemsFromModel(true);
  removeOldAccountFromDatabase(true, true);
  updateCounts(true);
  itemChanged({this});
  requestReloadMessageList(true);
}

QIcon ServiceRoot::feedIconForMessage(const QString& feed_custom_id) const {
  QString low_id = feed_custom_id.toLower();
  RootItem* found_item = getItemFromSubTree([low_id](const RootItem* it) {
    return it->kind() == RootItem::Kind::Feed && it->customId().toLower() == low_id;
  });

  if (found_item != nullptr) {
    return found_item->icon();
  }
  else {
    return QIcon();
  }
}

void ServiceRoot::removeOldAccountFromDatabase(bool delete_messages_too, bool delete_labels_too) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::deleteAccountData(database, accountId(), delete_messages_too, delete_labels_too);
}

void ServiceRoot::cleanAllItemsFromModel(bool clean_labels_too) {
  auto chi = childItems();

  for (RootItem* top_level_item : qAsConst(chi)) {
    if (top_level_item->kind() != RootItem::Kind::Bin && top_level_item->kind() != RootItem::Kind::Important &&
        top_level_item->kind() != RootItem::Kind::Unread && top_level_item->kind() != RootItem::Kind::Probes &&
        top_level_item->kind() != RootItem::Kind::Labels) {
      requestItemRemoval(top_level_item);
    }
  }

  if (labelsNode() != nullptr && clean_labels_too) {
    auto lbl_chi = labelsNode()->childItems();

    for (RootItem* lbl : qAsConst(lbl_chi)) {
      requestItemRemoval(lbl);
    }
  }
}

void ServiceRoot::appendCommonNodes() {
  if (recycleBin() != nullptr && !childItems().contains(recycleBin())) {
    appendChild(recycleBin());
  }

  if (importantNode() != nullptr && !childItems().contains(importantNode())) {
    appendChild(importantNode());
  }

  if (unreadNode() != nullptr && !childItems().contains(unreadNode())) {
    appendChild(unreadNode());
  }

  if (labelsNode() != nullptr && !childItems().contains(labelsNode())) {
    appendChild(labelsNode());
  }

  if (probesNode() != nullptr && !childItems().contains(probesNode())) {
    appendChild(probesNode());
  }
}

bool ServiceRoot::cleanFeeds(const QList<Feed*>& items, bool clean_read_only) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::cleanFeeds(database, textualFeedIds(items), clean_read_only, accountId())) {
    getParentServiceRoot()->updateCounts(true);
    getParentServiceRoot()->itemChanged(getParentServiceRoot()->getSubTree());
    getParentServiceRoot()->requestReloadMessageList(true);
    return true;
  }
  else {
    return false;
  }
}

void ServiceRoot::removeLeftOverMessages() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::purgeLeftoverMessages(database, accountId());
}

void ServiceRoot::removeLeftOverMessageFilterAssignments() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::purgeLeftoverMessageFilterAssignments(database, accountId());
}

QList<Message> ServiceRoot::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesForAccount(database, accountId());
}

bool ServiceRoot::supportsFeedAdding() const {
  return false;
}

bool ServiceRoot::supportsCategoryAdding() const {
  return false;
}

ServiceRoot::LabelOperation ServiceRoot::supportedLabelOperations() const {
  return LabelOperation::Adding | LabelOperation::Editing | LabelOperation::Deleting;
}

QString ServiceRoot::additionalTooltip() const {
  return tr("Number of feeds: %1\n"
            "Number of categories: %2")
    .arg(QString::number(getSubTreeFeeds().size()), QString::number(getSubTreeCategories().size()));
}

void ServiceRoot::saveAccountDataToDatabase() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::createOverwriteAccount(database, this);
  }
  catch (const ApplicationException& ex) {
    qFatal("Account was not saved into database: '%s'.", qPrintable(ex.message()));
  }
}

QVariantHash ServiceRoot::customDatabaseData() const {
  return {};
}

void ServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  Q_UNUSED(data)
}

bool ServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return false;
}

bool ServiceRoot::displaysEnclosures() const {
  return true;
}

void ServiceRoot::aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                           const QHash<QString, QHash<BagOfMessages, QStringList>>& stated_messages,
                                           const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(feeds)
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)
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

void ServiceRoot::requestItemsReassignment(const QList<RootItem*>& items, RootItem* new_parent) {
  for (RootItem* it : items) {
    requestItemReassignment(it, new_parent);
  }
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
  auto str = getSubTreeFeeds();

  for (const Feed* feed : qAsConst(str)) {
    QVariantMap feed_custom_data;

    // TODO: This could potentially call Feed::customDatabaseData() and append it
    // to this map and also subsequently restore, but the method is at this point
    // not really used by any syncable plugin.
    feed_custom_data.insert(QSL("auto_update_interval"), feed->autoUpdateInterval());
    feed_custom_data.insert(QSL("auto_update_type"), int(feed->autoUpdateType()));
    feed_custom_data.insert(QSL("msg_filters"), QVariant::fromValue(feed->messageFilters()));
    feed_custom_data.insert(QSL("is_off"), feed->isSwitchedOff());
    feed_custom_data.insert(QSL("is_quiet"), feed->isQuiet());
    feed_custom_data.insert(QSL("open_articles_directly"), feed->openArticlesDirectly());
    feed_custom_data.insert(QSL("is_rtl"), feed->isRtl());
    feed_custom_data.insert(QSL("add_any_datetime_articles"), feed->addAnyDatetimeArticles());
    feed_custom_data.insert(QSL("datetime_to_avoid"), feed->datetimeToAvoid().toMSecsSinceEpoch());

    // NOTE: This is here specifically to be able to restore custom sort order.
    // Otherwise the information is lost when list of feeds/folders is refreshed from remote
    // service.
    feed_custom_data.insert(QSL("sort_order"), feed->sortOrder());

    custom_data.insert(feed->customId(), feed_custom_data);
  }

  return custom_data;
}

QMap<QString, QVariantMap> ServiceRoot::storeCustomCategoriesData() {
  QMap<QString, QVariantMap> custom_data;
  auto str = getSubTreeCategories();

  for (const Category* cat : qAsConst(str)) {
    QVariantMap cat_custom_data;

    // NOTE: This is here specifically to be able to restore custom sort order.
    // Otherwise the information is lost when list of feeds/folders is refreshed from remote
    // service.
    cat_custom_data.insert(QSL("sort_order"), cat->sortOrder());

    custom_data.insert(cat->customId(), cat_custom_data);
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

      feed->setAutoUpdateInterval(feed_custom_data.value(QSL("auto_update_interval")).toInt());
      feed
        ->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(feed_custom_data.value(QSL("auto_update_type")).toInt()));
      feed->setMessageFilters(feed_custom_data.value(QSL("msg_filters")).value<QList<QPointer<MessageFilter>>>());

      feed->setIsSwitchedOff(feed_custom_data.value(QSL("is_off")).toBool());
      feed->setIsQuiet(feed_custom_data.value(QSL("is_quiet")).toBool());
      feed->setOpenArticlesDirectly(feed_custom_data.value(QSL("open_articles_directly")).toBool());
      feed->setIsRtl(feed_custom_data.value(QSL("is_rtl")).toBool());
      feed->setAddAnyDatetimeArticles(feed_custom_data.value(QSL("add_any_datetime_articles")).toBool());
      feed->setDatetimeToAvoid(TextFactory::parseDateTime(feed_custom_data.value(QSL("datetime_to_avoid"))
                                                            .value<qint64>()));

      // qDebugNN << feed->datetimeToAvoid().toString();
    }
  }
}

void ServiceRoot::restoreCustomCategoriesData(const QMap<QString, QVariantMap>& data,
                                              const QHash<QString, Category*>& cats) {
  Q_UNUSED(data)
  Q_UNUSED(cats)
}

QNetworkProxy ServiceRoot::networkProxy() const {
  return m_networkProxy;
}

void ServiceRoot::setNetworkProxy(const QNetworkProxy& network_proxy) {
  m_networkProxy = network_proxy;

  emit proxyChanged(network_proxy);
}

ImportantNode* ServiceRoot::importantNode() const {
  return m_importantNode;
}

LabelsNode* ServiceRoot::labelsNode() const {
  return m_labelsNode;
}

SearchsNode* ServiceRoot::probesNode() const {
  return m_probesNode;
}

UnreadNode* ServiceRoot::unreadNode() const {
  return m_unreadNode;
}

void ServiceRoot::onDatabaseCleanup() {}

void ServiceRoot::syncIn() {
  QIcon original_icon = icon();

  setIcon(qApp->icons()->fromTheme(QSL("view-refresh")));
  itemChanged({this});

  try {
    qDebugNN << LOGSEC_CORE << "Starting sync-in process.";

    RootItem* new_tree = obtainNewTreeForSyncIn();

    qDebugNN << LOGSEC_CORE << "New feed tree for sync-in obtained.";

    auto feed_custom_data = storeCustomFeedsData();
    auto categories_custom_data = storeCustomCategoriesData();

    // Remove from feeds model, then from SQL but leave messages intact.
    bool uses_remote_labels = Globals::hasFlag(supportedLabelOperations(), LabelOperation::Synchronised);

    // Remove stuff.
    cleanAllItemsFromModel(uses_remote_labels);
    removeOldAccountFromDatabase(false, uses_remote_labels);

    // Re-sort items to accomodate current sort order.
    resortAccountTree(new_tree, categories_custom_data, feed_custom_data);

    // Restore some local settings to feeds etc.
    restoreCustomCategoriesData(categories_custom_data, new_tree->getHashedSubTreeCategories());
    restoreCustomFeedsData(feed_custom_data, new_tree->getHashedSubTreeFeeds());

    // Model is clean, now store new tree into DB and
    // set primary IDs of the items.
    DatabaseQueries::storeAccountTree(qApp->database()->driver()->connection(metaObject()->className()),
                                      new_tree,
                                      accountId());

    // We have new feed, some feeds were maybe removed,
    // so remove left over messages and filter assignments.
    removeLeftOverMessages();
    removeLeftOverMessageFilterAssignments();

    auto chi = new_tree->childItems();

    for (RootItem* top_level_item : qAsConst(chi)) {
      if (top_level_item->kind() != Kind::Labels) {
        top_level_item->setParent(nullptr);
        requestItemReassignment(top_level_item, this);
      }
      else {
        // It seems that some labels got synced-in.
        if (labelsNode() != nullptr) {
          auto lbl_chi = top_level_item->childItems();

          for (RootItem* new_lbl : qAsConst(lbl_chi)) {
            new_lbl->setParent(nullptr);
            requestItemReassignment(new_lbl, labelsNode());
          }
        }
      }
    }

    new_tree->clearChildren();
    new_tree->deleteLater();

    updateCounts(true);
    requestReloadMessageList(true);
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_CORE << "New feed tree for sync-in NOT obtained:" << QUOTE_W_SPACE_DOT(ex.message());

    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("Error when fetching list of feeds"),
                                    tr("Feeds & categories for account '%1' were not fetched, error: %2")
                                      .arg(title(), ex.message()),
                                    QSystemTrayIcon::MessageIcon::Critical),
                         GuiMessageDestination(true, true));
  }

  setIcon(original_icon);
  itemChanged(getSubTree());
  requestItemExpand(getSubTree(), true);
}

void ServiceRoot::performInitialAssembly(const Assignment& categories,
                                         const Assignment& feeds,
                                         const QList<Label*>& labels,
                                         const QList<Search*>& probes) {
  assembleCategories(categories);
  assembleFeeds(feeds);
  labelsNode()->loadLabels(labels);
  probesNode()->loadProbes(probes);

  updateCounts(true);
}

RootItem* ServiceRoot::obtainNewTreeForSyncIn() const {
  return nullptr;
}

QStringList ServiceRoot::customIDSOfMessagesForItem(RootItem* item, ReadStatus target_read) {
  if (item->getParentServiceRoot() != this) {
    // Not item from this account.
    return {};
  }
  else {
    QStringList list;

    switch (item->kind()) {
      case RootItem::Kind::Labels:
      case RootItem::Kind::Probes:
      case RootItem::Kind::Category: {
        auto chi = item->childItems();

        for (RootItem* child : qAsConst(chi)) {
          list.append(customIDSOfMessagesForItem(child, target_read));
        }

        return list;
      }

      case RootItem::Kind::Label: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromLabel(database, item->toLabel(), target_read);
        break;
      }

      case RootItem::Kind::Probe: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromProbe(database, item->toProbe(), target_read);
        break;
      }

      case RootItem::Kind::ServiceRoot: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromAccount(database, target_read, accountId());
        break;
      }

      case RootItem::Kind::Bin: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromBin(database, target_read, accountId());
        break;
      }

      case RootItem::Kind::Feed: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfMessagesFromFeed(database, item->customId(), target_read, accountId());
        break;
      }

      case RootItem::Kind::Important: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfImportantMessages(database, target_read, accountId());
        break;
      }

      case RootItem::Kind::Unread: {
        QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

        list = DatabaseQueries::customIdsOfUnreadMessages(database, accountId());
        break;
      }

      default:
        break;
    }

    qDebugNN << LOGSEC_CORE << "Custom IDs of messages for some operation are:" << QUOTE_W_SPACE_DOT(list);
    return list;
  }
}

bool ServiceRoot::markFeedsReadUnread(const QList<Feed*>& items, RootItem::ReadStatus read) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::markFeedsReadUnread(database, textualFeedIds(items), accountId(), read)) {
    getParentServiceRoot()->updateCounts(false);
    getParentServiceRoot()->itemChanged(getParentServiceRoot()->getSubTree());
    getParentServiceRoot()->requestReloadMessageList(read == RootItem::ReadStatus::Read);
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
    stringy_urls.append(!feed->source().isEmpty() ? feed->source() : QSL("no-url"));
  }

  return stringy_urls;
}

QStringList ServiceRoot::textualFeedIds(const QList<Feed*>& feeds) const {
  QStringList stringy_ids;
  stringy_ids.reserve(feeds.size());

  for (const Feed* feed : feeds) {
    stringy_ids.append(QSL("'%1'").arg(feed->customId()));
  }

  return stringy_ids;
}

QStringList ServiceRoot::customIDsOfMessages(const QList<ImportanceChange>& changes) {
  QSet<QString> list;
  list.reserve(changes.size());

  for (const auto& change : changes) {
    list.insert(change.first.m_customId);
  }

  return list.values();
}

QStringList ServiceRoot::customIDsOfMessages(const QList<Message>& messages) {
  QSet<QString> list;
  list.reserve(messages.size());

  for (const Message& message : messages) {
    list.insert(message.m_customId);
  }

  return list.values();
}

int ServiceRoot::accountId() const {
  return m_accountId;
}

void ServiceRoot::setAccountId(int account_id) {
  m_accountId = account_id;

  auto* cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->setUniqueId(account_id);
  }
}

bool ServiceRoot::loadMessagesForItem(RootItem* item, MessagesModel* model) {
  if (item->kind() == RootItem::Kind::Bin) {
    model->setFilter(QSL("Messages.is_deleted = 1 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1")
                       .arg(QString::number(accountId())));
  }
  else if (item->kind() == RootItem::Kind::Important) {
    model->setFilter(QSL("Messages.is_important = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND "
                         "Messages.account_id = %1")
                       .arg(QString::number(accountId())));
  }
  else if (item->kind() == RootItem::Kind::Unread) {
    model->setFilter(QSL("Messages.is_read = 0 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND "
                         "Messages.account_id = %1")
                       .arg(QString::number(accountId())));
  }
  else if (item->kind() == RootItem::Kind::Probe) {
    item->updateCounts(true);
    itemChanged({item});

    model->setFilter(QSL("Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1 AND "
                         "(Messages.title REGEXP '%2' OR Messages.contents REGEXP '%2')")
                       .arg(QString::number(accountId()), item->toProbe()->filter()));
  }
  else if (item->kind() == RootItem::Kind::Label) {
    // Show messages with particular label.
    model->setFilter(QSL("Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND "
                         "Messages.labels LIKE \"%.%2.%\" AND Messages.account_id = %1")
                       .arg(QString::number(accountId()), item->customId()));
  }
  else if (item->kind() == RootItem::Kind::Labels) {
    // Show messages with any label.
    model->setFilter(QSL("Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND "
                         "LENGTH(Messages.labels) > 2 AND Messages.account_id = %1")
                       .arg(QString::number(accountId())));
  }
  else if (item->kind() == RootItem::Kind::ServiceRoot) {
    model->setFilter(QSL("Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = %1")
                       .arg(QString::number(accountId())));

    qDebugNN << LOGSEC_CORE << "Displaying messages from account:" << QUOTE_W_SPACE_DOT(accountId());
  }
  else if (item->kind() == RootItem::Kind::Probes) {
    model->setFilter(QSL(DEFAULT_SQL_MESSAGES_FILTER));

    qWarningNN << LOGSEC_CORE << "Showing of all regex queries combined is not supported.";
  }
  else {
    QList<Feed*> children = item->getSubTreeFeeds();
    QString filter_clause = textualFeedIds(children).join(QSL(", "));

    if (filter_clause.isEmpty()) {
      filter_clause = QSL("null");
    }

    model->setFilter(QSL("Feeds.custom_id IN (%1) AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND "
                         "Messages.account_id = %2")
                       .arg(filter_clause, QString::number(accountId())));

    QString urls = textualFeedUrls(children).join(QSL(", "));

    qDebugNN << LOGSEC_CORE << "Displaying messages from feeds IDs:" << QUOTE_W_SPACE(filter_clause)
             << "and URLs:" << QUOTE_W_SPACE_DOT(urls);
  }

  return true;
}

bool ServiceRoot::onBeforeSetMessagesRead(RootItem* selected_item,
                                          const QList<Message>& messages,
                                          RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(customIDsOfMessages(messages), read);
  }

  return true;
}

bool ServiceRoot::onAfterSetMessagesRead(RootItem* selected_item,
                                         const QList<Message>& messages,
                                         RootItem::ReadStatus read) {
  Q_UNUSED(messages)
  Q_UNUSED(read)

  // We know that some messages were marked as read or unread, therefore we do not need to recount
  // all items, but only some:
  //  - recycle bin (if recycle bin IS selected)
  //  - feeds of those messages (if recycle bin is NOT selected)
  //  - important articles (if some messages IS important AND recycle bin is NOT selected)
  //  - unread articles (if some messages IS unread AND recycle bin is NOT selected)
  //  - labels assigned to articles (if recycle bin is NOT selected)
  //  - probes (if recycle bin is NOT selected)
  QList<RootItem*> to_update;

  if (selected_item->kind() == RootItem::Kind::Bin) {
    selected_item->updateCounts(false);
    to_update << selected_item;
  }
  else {
    auto linq = boolinq::from(messages);

    // 1. Feeds of messages.
    auto feed_ids = linq
                      .select([](const Message& msg) {
                        return msg.m_feedId;
                      })
                      .distinct()
                      .toStdList();

    for (const QString& feed_id : feed_ids) {
      auto* feed = getItemFromSubTree([feed_id](const RootItem* it) {
        return it->kind() == RootItem::Kind::Feed && it->customId() == feed_id;
      });

      if (feed != nullptr) {
        feed->updateCounts(false);
        to_update << feed;
      }
    }

    // 2. Important.
    if (importantNode() != nullptr) {
      if (linq.any([](const Message& msg) {
            return msg.m_isImportant;
          })) {
        importantNode()->updateCounts(false);
        to_update << importantNode();
      }
    }

    // 3. Unread.
    if (unreadNode() != nullptr) {
      unreadNode()->updateCounts(false);
      to_update << unreadNode();
    }

    // 4. Labels assigned.
    if (labelsNode() != nullptr) {
      auto db = qApp->database()->driver()->connection(metaObject()->className());
      QStringList lbls; // = DatabaseQueries::getCountOfAssignedLabelsToMessages(db, messages, accountId());

      for (const Message& msg : messages) {
        for (const QString& lbl : msg.m_assignedLabelsIds) {
          if (!lbls.contains(lbl)) {
            lbls.append(lbl);
          }
        }
      }

      for (const QString& lbl : lbls) {
        Label* l = labelsNode()->labelById(lbl);
        l->updateCounts(false);
        to_update << l;
      }
    }

    // 5. Probes.
    m_probesNode->updateCounts(false);
    to_update << m_probesNode->childItems();
  }

  itemChanged(to_update);
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

  // NOTE: We know that some messages were marked as starred or unstarred. Starred count
  // is not displayed anywhere in feed list except "Important articles" item.
  auto in = importantNode();

  if (in != nullptr) {

    in->updateCounts(true);
    itemChanged({in});
  }

  return true;
}

bool ServiceRoot::onBeforeMessagesDelete(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)
  return true;
}

bool ServiceRoot::onAfterMessagesDelete(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)

  // TODO: We know that some messages were deleted, therefore we do not need to recount
  // all items, but only some:
  //  - feeds of those messages (if recycle bin is NOT selected)
  //  - recycle bin (if recycle bin IS selected)
  //  - important articles (if some message IS important AND recycle bin is NOT selected)
  //  - unread articles (if some messages IS unread AND if recycle bin is NOT selected)
  //  - labels assigned to articles (if recycle bin is NOT selected)

  updateCounts(true);
  itemChanged(getSubTree());
  return true;
}

bool ServiceRoot::onBeforeLabelMessageAssignmentChanged(const QList<Label*>& labels,
                                                        const QList<Message>& messages,
                                                        bool assign) {
  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    boolinq::from(labels).for_each([cache, messages, assign](Label* lbl) {
      cache->addLabelsAssignmentsToCache(messages, lbl, assign);
    });
  }

  return true;
}

bool ServiceRoot::onAfterLabelMessageAssignmentChanged(const QList<Label*>& labels,
                                                       const QList<Message>& messages,
                                                       bool assign) {
  Q_UNUSED(messages)
  Q_UNUSED(assign)

  for (Label* lbl : labels) {
    lbl->updateCounts(true);
  };

  auto list = boolinq::from(labels)
                .select([](Label* lbl) {
                  return static_cast<RootItem*>(lbl);
                })
                .toStdList();

  getParentServiceRoot()->itemChanged(FROM_STD_LIST(QList<RootItem*>, list));
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

CacheForServiceRoot* ServiceRoot::toCache() const {
  return dynamic_cast<CacheForServiceRoot*>(const_cast<ServiceRoot*>(this));
}

void ServiceRoot::assembleFeeds(const Assignment& feeds) {
  QHash<int, Category*> categories = getSubTreeCategoriesForAssemble();

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
      qWarningNN << LOGSEC_CORE << "Feed" << QUOTE_W_SPACE(feed.second->title()) << "is loose, skipping it.";
    }
  }
}

void ServiceRoot::resortAccountTree(RootItem* tree,
                                    const QMap<QString, QVariantMap>& custom_category_data,
                                    const QMap<QString, QVariantMap>& custom_feed_data) const {
  // Iterate tree and rearrange children.
  QList<RootItem*> traversable_items;

  traversable_items.append(tree);

  while (!traversable_items.isEmpty()) {
    auto* root = traversable_items.takeFirst();
    auto& chldr = root->childItems();

    // Sort children so that we are sure that feeds are sorted with sort order
    // other item types do not matter.
    std::sort(chldr.begin(), chldr.end(), [&](const RootItem* lhs, const RootItem* rhs) {
      if (lhs->kind() == RootItem::Kind::Feed && rhs->kind() == RootItem::Kind::Feed) {
        auto lhs_order = custom_feed_data[lhs->customId()].value(QSL("sort_order")).toInt();
        auto rhs_order = custom_feed_data[rhs->customId()].value(QSL("sort_order")).toInt();

        return lhs_order < rhs_order;
      }
      else if (lhs->kind() == RootItem::Kind::Category && rhs->kind() == RootItem::Kind::Category) {
        auto lhs_order = custom_category_data[lhs->customId()].value(QSL("sort_order")).toInt();
        auto rhs_order = custom_category_data[rhs->customId()].value(QSL("sort_order")).toInt();

        return lhs_order < rhs_order;
      }
      else {
        return lhs->kind() < rhs->kind();
      }
    });

    traversable_items.append(root->childItems());
  }
}

void ServiceRoot::assembleCategories(const Assignment& categories) {
  Assignment editable_categories = categories;
  QHash<int, RootItem*> assignments;

  assignments.insert(NO_PARENT_CATEGORY, this);

  // Add top-level categories.
  while (!editable_categories.isEmpty()) {
    for (int i = 0; i < editable_categories.size(); i++) {
      if (assignments.contains(editable_categories.at(i).first)) {
        // Parent category of this category is already added.
        assignments.value(editable_categories.at(i).first)->appendChild(editable_categories.at(i).second);

        // Now, added category can be parent for another categories, add it.
        assignments.insert(editable_categories.at(i).second->id(), editable_categories.at(i).second);

        // Remove the category from the list, because it was
        // added to the final collection.
        editable_categories.removeAt(i);
        i--;
      }
    }
  }
}

ServiceRoot::LabelOperation operator|(ServiceRoot::LabelOperation lhs, ServiceRoot::LabelOperation rhs) {
  return static_cast<ServiceRoot::LabelOperation>(static_cast<char>(lhs) | static_cast<char>(rhs));
}

ServiceRoot::LabelOperation operator&(ServiceRoot::LabelOperation lhs, ServiceRoot::LabelOperation rhs) {
  return static_cast<ServiceRoot::LabelOperation>(static_cast<char>(lhs) & static_cast<char>(rhs));
}

UpdatedArticles ServiceRoot::updateMessages(QList<Message>& messages, Feed* feed, bool force_update, QMutex* db_mutex) {
  UpdatedArticles updated_messages;

  if (messages.isEmpty()) {
    qDebugNN << "No messages to be updated/added in DB for feed" << QUOTE_W_SPACE_DOT(feed->customId());
    return updated_messages;
  }

  bool ok = false;
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

  qDebugNN << LOGSEC_CORE << "Updating messages in DB.";

  updated_messages = DatabaseQueries::updateMessages(database, messages, feed, force_update, db_mutex, &ok);

  if (!updated_messages.m_unread.isEmpty() || !updated_messages.m_all.isEmpty()) {
    QMutexLocker lck(db_mutex);

    // Something was added or updated in the DB, update numbers.
    feed->updateCounts(true);

    if (recycleBin() != nullptr) {
      recycleBin()->updateCounts(true);
    }

    if (importantNode() != nullptr) {
      importantNode()->updateCounts(true);
    }

    if (unreadNode() != nullptr) {
      unreadNode()->updateCounts(true);
    }

    if (labelsNode() != nullptr) {
      labelsNode()->updateCounts(true);
    }

    if (probesNode() != nullptr) {
      probesNode()->updateCounts(true);
    }
  }

  // NOTE: Do not update model items here. We update only once when all feeds are fetched.
  return updated_messages;
}
