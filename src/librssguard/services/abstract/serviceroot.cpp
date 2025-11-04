// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/serviceroot.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/messagesmodel.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/gui/custommessagepreviewer.h"
#include "services/abstract/gui/formaccountdetails.h"
#include "services/abstract/gui/formaddeditlabel.h"
#include "services/abstract/gui/formaddeditprobe.h"
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
    m_accountId(NO_PARENT_CATEGORY), m_networkProxy(QNetworkProxy()), m_nodeShowUnread(true), m_nodeShowImportant(true),
    m_nodeShowLabels(true), m_nodeShowProbes(true) {
  setKind(RootItem::Kind::ServiceRoot);
  appendCommonNodes();
}

ServiceRoot::~ServiceRoot() {}

void ServiceRoot::deleteItem() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::deleteAccount(database, this);
  stop();
  requestItemRemoval(this);
}

void ServiceRoot::editItems(const QList<RootItem*>& items) {
  // Feed editing.
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
    return;
  }

  // Category editing.
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

    form_pointer->addEditCategory<Category>(FROM_STD_LIST(QList<Category*>, std_categories));
    return;
  }

  // Label editing.
  auto std_labels = boolinq::from(items)
                      .select([](RootItem* it) {
                        return qobject_cast<Label*>(it);
                      })
                      .where([](Label* fd) {
                        return fd != nullptr;
                      })
                      .toStdList();

  if (std_labels.size() == 1) {
    // Support editing labels one by one.
    FormAddEditLabel form(qApp->mainFormWidget());
    Label* lbl = std_labels.front();

    if (form.execForEdit(lbl)) {
      QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

      try {
        DatabaseQueries::updateLabel(db, lbl);
        itemChanged({lbl});
      }
      catch (const ApplicationException& ex) {
        qCriticalNN << LOGSEC_CORE << "Failed to update label:" << NONQUOTE_W_SPACE_DOT(ex.message());
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             GuiMessage(tr("Cannot update label"),
                                        tr("Failed to update label with new information: %1.").arg(ex.message()),
                                        QSystemTrayIcon::MessageIcon::Critical),
                             GuiMessageDestination(true, true));
      }
    }

    return;
  }

  // Probe editing.
  auto std_probes = boolinq::from(items)
                      .select([](RootItem* it) {
                        return qobject_cast<Search*>(it);
                      })
                      .where([](Search* fd) {
                        return fd != nullptr;
                      })
                      .toStdList();

  if (std_probes.size() == 1) {
    // Support editing probes one by one.
    FormAddEditProbe form(qApp->mainFormWidget());
    Search* probe = std_probes.front();

    if (form.execForEdit(probe)) {
      QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

      try {
        DatabaseQueries::updateProbe(db, probe);
        itemChanged({probe});
      }
      catch (const ApplicationException& ex) {
        qCriticalNN << LOGSEC_CORE << "Failed to update probe:" << NONQUOTE_W_SPACE_DOT(ex.message());
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             GuiMessage(tr("Cannot update probe item"),
                                        tr("Failed to update selected probe: %1.").arg(ex.message()),
                                        QSystemTrayIcon::MessageIcon::Critical),
                             GuiMessageDestination(true, true));
      }
    }

    return;
  }

  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       {tr("Unsupported"), tr("This is not suppported (yet)."), QSystemTrayIcon::MessageIcon::Warning});
}

void ServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);
  DatabaseQueries::markAccountReadUnread(qApp->database()->driver()->connection(metaObject()->className()),
                                         service->accountId(),
                                         status);
  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}

void ServiceRoot::cleanMessages(bool clean_read_only) {
  cleanFeeds(getSubTreeFeeds(), clean_read_only);
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
  auto str = getSubTree<RootItem>();

  for (RootItem* child : std::as_const(str)) {
    if (child->kind() == RootItem::Kind::Feed) {
      feeds.append(child->toFeed());
    }
    else if (child->kind() == RootItem::Kind::Unread) {
      child->updateCounts(true);
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
  auto counts = DatabaseQueries::getMessageCountsForAccount(database, accountId(), including_total_count);

  for (Feed* feed : feeds) {
    if (counts.contains(feed->id())) {
      feed->setCountOfUnreadMessages(counts.value(feed->id()).m_unread);

      if (including_total_count) {
        feed->setCountOfAllMessages(counts.value(feed->id()).m_total);
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

bool ServiceRoot::canBeDeleted() const {
  return true;
}

void ServiceRoot::completelyRemoveAllData() {
  // Purge old data from SQL and clean all model items.
  cleanAllItemsFromModel(true);
  removeOldAccountFromDatabase(true, true);
  updateCounts(true);
  itemChanged({this});
  informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

void ServiceRoot::removeOldAccountFromDatabase(bool delete_messages_too, bool delete_labels_too) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::deleteAccountData(database, accountId(), delete_messages_too, delete_labels_too);
}

void ServiceRoot::cleanAllItemsFromModel(bool clean_labels_too) {
  auto chi = childItems();

  for (RootItem* top_level_item : std::as_const(chi)) {
    if (top_level_item->kind() != RootItem::Kind::Bin && top_level_item->kind() != RootItem::Kind::Important &&
        top_level_item->kind() != RootItem::Kind::Unread && top_level_item->kind() != RootItem::Kind::Probes &&
        top_level_item->kind() != RootItem::Kind::Labels) {
      requestItemRemoval(top_level_item);
    }
  }

  if (labelsNode() != nullptr && clean_labels_too) {
    auto lbl_chi = labelsNode()->childItems();

    for (RootItem* lbl : std::as_const(lbl_chi)) {
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

void ServiceRoot::cleanFeeds(const QList<Feed*>& items, bool clean_read_only) {
  ServiceRoot* service = account();

  service->onBeforeMessagesDelete(this, {});
  DatabaseQueries::cleanFeeds(qApp->database()->driver()->connection(metaObject()->className()),
                              textualFeedIds(items),
                              clean_read_only,
                              accountId());
  service->onAfterMessagesDelete(this, {});
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
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
            "Number of categories: %2\n"
            "Number of disabled feeds: %3")
    .arg(QString::number(getSubTreeFeeds().size()),
         QString::number(getSubTreeCategories().size()),
         QString::number(getSubTree<RootItem>([](const RootItem* ri) {
                           return ri->kind() == RootItem::Kind::Feed && ri->toFeed()->isSwitchedOff();
                         }).size()));
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
  return {{QSL("show_node_unread"), m_nodeShowUnread},
          {QSL("show_node_important"), m_nodeShowImportant},
          {QSL("show_node_labels"), m_nodeShowLabels},
          {QSL("show_node_probes"), m_nodeShowProbes}};
}

void ServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_nodeShowUnread = data.value(QSL("show_node_unread"), true).toBool();
  m_nodeShowImportant = data.value(QSL("show_node_important"), true).toBool();
  m_nodeShowLabels = data.value(QSL("show_node_labels"), true).toBool();
  m_nodeShowProbes = data.value(QSL("show_node_probes"), true).toBool();
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

void ServiceRoot::informOthersAboutDataChange(RootItem* item, FeedsModel::ExternalDataChange change) {
  emit dataChangeNotificationTriggered(item, change);
}

void ServiceRoot::requestItemExpand(const QList<RootItem*>& items, bool expand) {
  emit itemExpandRequested(items, expand);
}

void ServiceRoot::requestItemExpandStateSave(RootItem* subtree_root) {
  emit itemExpandStateSaveRequested(subtree_root);
}

void ServiceRoot::requestItemReassignment(RootItem* item, RootItem* new_parent, bool blocking) {
  if (blocking) {
    emit itemBlockingReassignmentRequested(item, new_parent);
  }
  else {
    emit itemReassignmentRequested(item, new_parent);
  }
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

  for (const Feed* feed : std::as_const(str)) {
    QVariantMap feed_custom_data;

    feed_custom_data.insert(QSL("id"), feed->id());
    feed_custom_data.insert(QSL("auto_update_interval"), feed->autoUpdateInterval());
    feed_custom_data.insert(QSL("auto_update_type"), int(feed->autoUpdateType()));
    feed_custom_data.insert(QSL("msg_filters"), QVariant::fromValue(feed->messageFilters()));
    feed_custom_data.insert(QSL("is_off"), feed->isSwitchedOff());
    feed_custom_data.insert(QSL("is_quiet"), feed->isQuiet());
    feed_custom_data.insert(QSL("is_rtl"), QVariant::fromValue(feed->rtlBehavior()));
    feed_custom_data.insert(QSL("article_limit_ignore"), QVariant::fromValue(feed->articleIgnoreLimit()));

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

  for (const Category* cat : std::as_const(str)) {
    QVariantMap cat_custom_data;

    cat_custom_data.insert(QSL("id"), cat->id());

    // NOTE: This is here specifically to be able to restore custom sort order.
    // Otherwise the information is lost when list of feeds/folders is refreshed from remote
    // service.
    cat_custom_data.insert(QSL("sort_order"), cat->sortOrder());

    custom_data.insert(cat->customId(), cat_custom_data);
  }

  return custom_data;
}

QMap<QString, QVariantMap> ServiceRoot::storeCustomLabelsData() {
  QMap<QString, QVariantMap> custom_data;
  QList<Label*> str = m_labelsNode == nullptr ? QList<Label*>() : m_labelsNode->labels();

  for (const Label* lbl : std::as_const(str)) {
    QVariantMap lbl_custom_data;

    lbl_custom_data.insert(QSL("id"), lbl->id());

    custom_data.insert(lbl->customId(), lbl_custom_data);
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

      feed->setId(feed_custom_data.value(QSL("id")).toInt());
      feed->setAutoUpdateInterval(feed_custom_data.value(QSL("auto_update_interval")).toInt());
      feed
        ->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(feed_custom_data.value(QSL("auto_update_type")).toInt()));
      feed->setMessageFilters(feed_custom_data.value(QSL("msg_filters")).value<QList<QPointer<MessageFilter>>>());

      feed->setIsSwitchedOff(feed_custom_data.value(QSL("is_off")).toBool());
      feed->setIsQuiet(feed_custom_data.value(QSL("is_quiet")).toBool());
      feed->setRtlBehavior(feed_custom_data.value(QSL("is_rtl")).value<RtlBehavior>());

      feed
        ->setArticleIgnoreLimit(feed_custom_data.value(QSL("article_limit_ignore")).value<Feed::ArticleIgnoreLimit>());

      /*
      feed->setAddAnyDatetimeArticles(feed_custom_data.value(QSL("add_any_datetime_articles")).toBool());

      qint64 time_to_avoid = feed_custom_data.value(QSL("datetime_to_avoid")).value<qint64>();

      if (time_to_avoid > 10000) {
        feed->setDatetimeToAvoid(TextFactory::parseDateTime(time_to_avoid));
      }
      else {
        feed->setHoursToAvoid(time_to_avoid);
      }
      */
    }
  }
}

void ServiceRoot::restoreCustomCategoriesData(const QMap<QString, QVariantMap>& data,
                                              const QHash<QString, Category*>& cats) {
  Q_UNUSED(data)
  Q_UNUSED(cats)
}

void ServiceRoot::restoreCustomLabelsData(const QMap<QString, QVariantMap>& data, LabelsNode* labels) {
  if (data.isEmpty() || labels == nullptr) {
    return;
  }

  for (Label* lbl : labels->labels()) {
    if (data.contains(lbl->customId())) {
      QVariantMap lbl_custom_data = data.value(lbl->customId());

      lbl->setId(lbl_custom_data.value(QSL("id")).toInt());
    }
  }
}

bool ServiceRoot::nodeShowProbes() const {
  return m_nodeShowProbes;
}

void ServiceRoot::setNodeShowProbes(bool enabled) {
  m_nodeShowProbes = enabled;
}

bool ServiceRoot::nodeShowLabels() const {
  return m_nodeShowLabels;
}

void ServiceRoot::setNodeShowLabels(bool enabled) {
  m_nodeShowLabels = enabled;
}

bool ServiceRoot::nodeShowImportant() const {
  return m_nodeShowImportant;
}

void ServiceRoot::setNodeShowImportant(bool enabled) {
  m_nodeShowImportant = enabled;
}

bool ServiceRoot::nodeShowUnread() const {
  return m_nodeShowUnread;
}

void ServiceRoot::setNodeShowUnread(bool enabled) {
  m_nodeShowUnread = enabled;
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

FormAccountDetails* ServiceRoot::accountSetupDialog() const {
  return nullptr;
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

    // Remove from feeds model, then from SQL but leave messages intact.
    bool uses_remote_labels = Globals::hasFlag(supportedLabelOperations(), LabelOperation::Synchronised);

    auto feed_custom_data = storeCustomFeedsData();
    auto categories_custom_data = storeCustomCategoriesData();
    auto label_custom_data = uses_remote_labels ? storeCustomLabelsData() : QMap<QString, QVariantMap>();

    // Remove stuff.
    cleanAllItemsFromModel(uses_remote_labels);

    // NOTE: We need these primary IDs because - all feed/article/label foreign keys
    // are realised via physical database ID (not custom ID, this is due to performance).
    // We need to re-use all existing IDs to retain proper foreign key relations to articles and labels.
    // So we need to make sure that temporarily vacant ID is not taken by newly created feed or label.
    QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());
    int next_primary_id_feeds = DatabaseQueries::highestPrimaryIdFeeds(db) + 1;
    int next_primary_id_labels = DatabaseQueries::highestPrimaryIdLabels(db) + 1;

    qApp->database()->driver()->setForeignKeyChecksDisabled(db);

    removeOldAccountFromDatabase(false, uses_remote_labels);

    // Re-sort items to accomodate current sort order.
    resortAccountTree(new_tree, categories_custom_data, feed_custom_data);

    // Restore some local settings to feeds etc.
    restoreCustomCategoriesData(categories_custom_data, new_tree->getHashedSubTreeCategories());
    restoreCustomFeedsData(feed_custom_data, new_tree->getHashedSubTreeFeeds());
    restoreCustomLabelsData(label_custom_data,
                            dynamic_cast<LabelsNode*>(new_tree->getItemFromSubTree([](const RootItem* it) {
                              return it->kind() == RootItem::Kind::Labels;
                            })));

    // Model is clean, now store new tree into DB and
    // set primary IDs of the items.
    DatabaseQueries::storeAccountTree(db, new_tree, next_primary_id_feeds, next_primary_id_labels, accountId());

    // We have new feed, some feeds were maybe removed,
    // so remove left over messages and filter assignments.
    DatabaseQueries::purgeLeftoverMessages(db, accountId());
    DatabaseQueries::purgeLeftoverMessageFilterAssignments(db, accountId());
    DatabaseQueries::purgeLeftoverLabelAssignments(db, accountId());

    qApp->database()->driver()->setForeignKeyChecksEnabled(db);

    auto chi = new_tree->childItems();

    for (RootItem* top_level_item : std::as_const(chi)) {
      if (top_level_item->kind() != Kind::Labels) {
        top_level_item->setParent(nullptr);
        requestItemReassignment(top_level_item, this);
      }
      else {
        // It seems that some labels got synced-in.
        if (labelsNode() != nullptr) {
          auto lbl_chi = top_level_item->childItems();

          for (RootItem* new_lbl : std::as_const(lbl_chi)) {
            new_lbl->setParent(nullptr);
            requestItemReassignment(new_lbl, labelsNode());
          }
        }
      }
    }

    new_tree->clearChildren();
    new_tree->deleteLater();

    updateCounts(true);
    informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::AccountSyncedIn);
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
  itemChanged(getSubTree<RootItem>());
  requestItemExpand(getSubTree<RootItem>(), true);
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

QStringList ServiceRoot::customIDsOfMessagesForItem(RootItem* item, ReadStatus target_read) {
  if (item->account() != this) {
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

        for (RootItem* child : std::as_const(chi)) {
          list.append(customIDsOfMessagesForItem(child, target_read));
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

        list = DatabaseQueries::customIdsOfMessagesFromFeed(database, item->id(), target_read, accountId());
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

    return list;
  }
}

QStringList ServiceRoot::textualFeedIds(const QList<Feed*>& feeds) const {
  QStringList stringy_ids;
  stringy_ids.reserve(feeds.size());

  for (const Feed* feed : feeds) {
    stringy_ids.append(QString::number(feed->id()));
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
    model->setFilter(DatabaseQueries::whereClauseBin(accountId()));
  }
  else if (item->kind() == RootItem::Kind::Important) {
    model->setFilter(DatabaseQueries::whereClauseImportantArticles(accountId()));
  }
  else if (item->kind() == RootItem::Kind::Unread) {
    model->setFilter(DatabaseQueries::whereClauseUnreadArticles(accountId()));
  }
  else if (item->kind() == RootItem::Kind::Probe) {
    model->setFilter(DatabaseQueries::whereClauseProbe(item->toProbe(), accountId()));
  }
  else if (item->kind() == RootItem::Kind::Label) {
    model->setFilter(DatabaseQueries::whereClauseLabel(item->id(), accountId()));
  }
  else if (item->kind() == RootItem::Kind::Labels) {
    model->setFilter(DatabaseQueries::whereClauseLabels(accountId()));
  }
  else if (item->kind() == RootItem::Kind::ServiceRoot) {
    model->setFilter(DatabaseQueries::whereClauseAccount(false, accountId()));
  }
  else if (item->kind() == RootItem::Kind::Probes) {
    model->setFilter(QSL(DEFAULT_SQL_MESSAGES_FILTER));

    qWarningNN << LOGSEC_CORE << "Showing of all queries combined is not supported.";
  }
  else {
    QList<Feed*> children = item->getSubTreeFeeds();
    QStringList feed_ids = textualFeedIds(children);

    model->setFilter(DatabaseQueries::whereClauseFeeds(feed_ids));
  }

  return true;
}

void ServiceRoot::onBeforeSetMessagesRead(RootItem* selected_item,
                                          const QStringList& message_custom_ids,
                                          ReadStatus read) {
  Q_UNUSED(selected_item)

  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(message_custom_ids, read);
  }
}

void ServiceRoot::onBeforeSetMessagesRead(RootItem* selected_item,
                                          const QList<Message>& messages,
                                          RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(customIDsOfMessages(messages), read);
  }
}

void ServiceRoot::onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes) {
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
}

void ServiceRoot::onAfterSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes) {
  Q_UNUSED(selected_item)
  Q_UNUSED(changes)

  // NOTE: We know that some messages were marked as starred or unstarred. Starred count
  // is not displayed anywhere in feed list except "Important articles" item.
  auto in = importantNode();

  if (in != nullptr) {
    in->updateCounts(true);
    itemChanged({in});
  }
}

void ServiceRoot::onBeforeMessagesDelete(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)
}

void ServiceRoot::onBeforeLabelMessageAssignmentChanged(const QList<Label*>& labels,
                                                        const QList<Message>& messages,
                                                        bool assign) {
  auto cache = dynamic_cast<CacheForServiceRoot*>(this);

  if (cache != nullptr) {
    boolinq::from(labels).for_each([cache, messages, assign](Label* lbl) {
      cache->addLabelsAssignmentsToCache(messages, lbl, assign);
    });
  }
}

void ServiceRoot::onAfterLabelMessageAssignmentChanged(const QList<Label*>& labels,
                                                       const QList<Message>& messages,
                                                       bool assign) {
  Q_UNUSED(messages)
  Q_UNUSED(assign)

  for (Label* lbl : labels) {
    lbl->updateCounts(true);
  };

  auto list = boolinq::from(labels)
                .select([](Label* lbl) {
                  return lbl;
                })
                .toStdList();

  account()->itemChanged(FROM_STD_LIST(QList<RootItem*>, list));
}

void ServiceRoot::onBeforeMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)
}

void ServiceRoot::refreshAfterArticlesChange(const QList<Message>& messages,
                                             bool refresh_bin,
                                             bool refresh_only_bin,
                                             bool including_total_counts) {
  if (refresh_only_bin) {
    m_recycleBin->updateCounts(true);
    itemChanged({m_recycleBin});
  }
  else {
    auto feeds_hashed = getPrimaryIdHashedSubTreeFeeds();
    auto msgs_linq = boolinq::from(messages);
    auto feed_ids = msgs_linq
                      .select([](const Message& msg) {
                        return msg.m_feedId;
                      })
                      .distinct()
                      .toStdVector();

    if (feed_ids.empty() || feed_ids.size() > 20) {
      updateCounts(including_total_counts);
      itemChanged({getSubTree<RootItem>()});
    }
    else {
      QList<RootItem*> to_update;

      for (int feed_id : feed_ids) {
        auto* fd = feeds_hashed.value(feed_id);
        fd->updateCounts(including_total_counts);

        to_update << fd;
      }

      if (m_importantNode != nullptr && msgs_linq.any([](const Message& msg) {
            return msg.m_isImportant;
          })) {
        m_importantNode->updateCounts(including_total_counts);
        to_update << m_importantNode;
      }

      if (m_unreadNode != nullptr) {
        m_unreadNode->updateCounts(true);
        to_update << m_unreadNode;
      }

      if (m_labelsNode != nullptr) {
        auto msg_lbls = msgs_linq
                          .selectMany([](const Message& msg) {
                            return boolinq::from(msg.m_assignedLabels);
                          })
                          .distinct()
                          .toStdVector();

        for (Label* lbl : msg_lbls) {
          if (lbl != nullptr) {
            lbl->updateCounts(including_total_counts);
            to_update << lbl;
          }
        }
      }

      if (refresh_bin) {
        m_recycleBin->updateCounts(including_total_counts);
        to_update << m_recycleBin;
      }

      itemChanged(to_update);
    }
  }
}

void ServiceRoot::onAfterMessagesRestoredFromBin(RootItem* selected_item, const QList<Message>& messages) {
  refreshAfterArticlesChange(messages, true, false, true);
}

void ServiceRoot::onAfterMessagesDelete(RootItem* selected_item, const QList<Message>& messages) {
  refreshAfterArticlesChange(messages,
                             true,
                             selected_item != nullptr && selected_item->kind() == RootItem::Kind::Bin,
                             true);
}

void ServiceRoot::onAfterSetMessagesRead(RootItem* selected_item,
                                         const QList<Message>& messages,
                                         RootItem::ReadStatus read) {
  refreshAfterArticlesChange(messages,
                             true,
                             selected_item != nullptr && selected_item->kind() == RootItem::Kind::Bin,
                             false);
}

void ServiceRoot::onAfterFeedsPurged(const QList<Feed*>& feeds) {
  Q_UNUSED(feeds)

  refreshAfterArticlesChange({}, false, false, true);
}

QNetworkProxy ServiceRoot::networkProxyForItem(RootItem* item) const {
  return networkProxy();
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

UpdatedArticles ServiceRoot::updateMessages(QList<Message>& messages,
                                            Feed* feed,
                                            bool force_update,
                                            bool recalculate_counts,
                                            QMutex* db_mutex) {
  UpdatedArticles updated_messages;
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

  if (!messages.isEmpty()) {
    qDebugNN << LOGSEC_CORE << "Updating messages in DB.";

    updated_messages = DatabaseQueries::updateMessages(database, messages, feed, force_update, false, db_mutex);
  }
  else {
    qDebugNN << "No messages to be updated/added in DB for feed" << QUOTE_W_SPACE_DOT(feed->customId());
  }

  bool anything_removed = feed->removeUnwantedArticles(database);

  if (recalculate_counts &&
      (anything_removed || !updated_messages.m_unread.isEmpty() || !updated_messages.m_all.isEmpty())) {
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

  // NOTE: Do not update model items here. We update only once when all feeds are fetched
  // or separately in downloader, if user has this enabled.
  return updated_messages;
}
