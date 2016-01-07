// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "gui/dialogs/formmain.h"
#include "network-web/networkfactory.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssrecyclebin.h"
#include "services/tt-rss/ttrsscategory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/gui/formeditaccount.h"
#include "services/tt-rss/gui/formeditfeed.h"

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QPointer>
#include <QPair>
#include <QClipboard>


TtRssServiceRoot::TtRssServiceRoot(RootItem *parent)
  : ServiceRoot(parent), m_recycleBin(new TtRssRecycleBin(this)),
    m_actionSyncIn(NULL), m_serviceMenu(QList<QAction*>()), m_network(new TtRssNetworkFactory) {
  setIcon(TtRssServiceEntryPoint().icon());
  setCreationDate(QDateTime::currentDateTime());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  delete m_network;
}

void TtRssServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)

  loadFromDatabase();

  if (childCount() == 1 && child(0)->kind() == RootItemKind::Bin) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  m_network->logout();

  qDebug("Stopping Tiny Tiny RSS account, logging out with result '%d'.", (int) m_network->lastError());
}

QString TtRssServiceRoot::code() {
  return SERVICE_CODE_TT_RSS;
}

bool TtRssServiceRoot::editViaGui() {
  QPointer<FormEditAccount> form_pointer = new FormEditAccount(qApp->mainForm());
  form_pointer.data()->execForEdit(this);
  delete form_pointer.data();
  return false;
}

bool TtRssServiceRoot::deleteViaGui() {
  QSqlDatabase connection = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(connection);

  query.setForwardOnly(true);
  query.prepare(QSL("DELETE FROM TtRssAccounts WHERE id = :id;"));
  query.bindValue(QSL(":id"), accountId());

  // Remove extra entry in "Tiny Tiny RSS accounts list" and then delete
  // all the categories/feeds and messages.
  if (query.exec()) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::markAsReadUnread(RootItem::ReadStatus status) {
  QStringList ids = customIDSOfMessagesForItem(this);
  TtRssUpdateArticleResponse response = m_network->updateArticles(ids, UpdateArticle::Unread,
                                                                  status == RootItem::Unread ?
                                                                    UpdateArticle::SetToTrue :
                                                                    UpdateArticle::SetToFalse);

  if (m_network->lastError() != QNetworkReply::NoError || response.updateStatus()  != STATUS_OK) {
    return false;
  }
  else {
    return ServiceRoot::markAsReadUnread(status);
  }
}

bool TtRssServiceRoot::supportsFeedAdding() const {
  return true;
}

bool TtRssServiceRoot::supportsCategoryAdding() const {
  return false;
}

void TtRssServiceRoot::addNewFeed(const QString &url) {
  QPointer<FormEditFeed> form_pointer = new FormEditFeed(this, qApp->mainForm());

  form_pointer.data()->execForAdd(url);
  delete form_pointer.data();
}

void TtRssServiceRoot::addNewCategory() {
  // Do nothing.
}

bool TtRssServiceRoot::canBeEdited() {
  return true;
}

bool TtRssServiceRoot::canBeDeleted() {
  return true;
}

QVariant TtRssServiceRoot::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return tr("Tiny Tiny RSS\n\nAccount ID: %3\nUsername: %1\nServer: %2\n"
                  "Last error: %4\nLast login on: %5").arg(m_network->username(),
                                                           m_network->url(),
                                                           QString::number(accountId()),
                                                           NetworkFactory::networkErrorText(m_network->lastError()),
                                                           m_network->lastLoginTime().isValid() ?
                                                             m_network->lastLoginTime().toString(Qt::DefaultLocaleShortDate) :
                                                             QSL("-"));
      }
      else {
        return ServiceRoot::data(column, role);
      }

    default:
      return ServiceRoot::data(column, role);
  }
}

QList<QAction*> TtRssServiceRoot::addItemMenu() {
  return QList<QAction*>();
}

RecycleBin *TtRssServiceRoot::recycleBin() {
  return m_recycleBin;
}

bool TtRssServiceRoot::loadMessagesForItem(RootItem *item, QSqlTableModel *model) {
  if (item->kind() == RootItemKind::Bin) {
    model->setFilter(QString("is_deleted = 1 AND is_pdeleted = 0 AND account_id = %1").arg(QString::number(accountId())));
  }
  else {
    QList<Feed*> children = item->getSubTreeFeeds();
    QString filter_clause = textualFeedIds(children).join(QSL(", "));

    model->setFilter(QString(QSL("feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = '%2'")).arg(filter_clause,
                                                                                                                   QString::number(accountId())));
    qDebug("Loading messages from feeds: %s.", qPrintable(filter_clause));
  }

  return true;
}

QList<QAction*> TtRssServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("item-sync")), tr("Sync in"), this);

    connect(m_actionSyncIn, SIGNAL(triggered()), this, SLOT(syncIn()));
    m_serviceMenu.append(m_actionSyncIn);
  }

  return m_serviceMenu;
}

QList<QAction*> TtRssServiceRoot::contextMenu() {
  return serviceMenu();
}

bool TtRssServiceRoot::onBeforeSetMessagesRead(RootItem *selected_item, const QList<Message> &messages,
                                               RootItem::ReadStatus read) {
  Q_UNUSED(selected_item)

  TtRssUpdateArticleResponse response = m_network->updateArticles(customIDsOfMessages(messages),
                                                                  UpdateArticle::Unread,
                                                                  read == RootItem::Unread ?
                                                                    UpdateArticle::SetToTrue :
                                                                    UpdateArticle::SetToFalse);

  if (m_network->lastError() == QNetworkReply::NoError && response.updateStatus() == STATUS_OK) {
    return true;
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::onAfterSetMessagesRead(RootItem *selected_item, const QList<Message> &messages, RootItem::ReadStatus read) {
  Q_UNUSED(messages)
  Q_UNUSED(read)

  selected_item->updateCounts(false);
  itemChanged(QList<RootItem*>() << selected_item);
  requestFeedReadFilterReload();
  return true;
}

bool TtRssServiceRoot::onBeforeSwitchMessageImportance(RootItem *selected_item, const QList<QPair<Message,Importance> > &changes) {
  Q_UNUSED(selected_item)

  // NOTE: We just toggle it here, because we know, that there is only
  // toggling of starred status supported by RSS Guard right now and
  // Tiny Tiny RSS API allows it, which is greate.
  TtRssUpdateArticleResponse response = m_network->updateArticles(customIDsOfMessages(changes),
                                                                  UpdateArticle::Starred,
                                                                  UpdateArticle::Togggle);

  if (m_network->lastError() == QNetworkReply::NoError && response.updateStatus() == STATUS_OK) {
    return true;
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::onAfterSwitchMessageImportance(RootItem *selected_item, const QList<QPair<Message,Importance> > &changes) {
  Q_UNUSED(selected_item)
  Q_UNUSED(changes)

  return true;
}

bool TtRssServiceRoot::onBeforeMessagesDelete(RootItem *selected_item, const QList<Message> &messages) {
  Q_UNUSED(selected_item)
  Q_UNUSED(messages)

  return true;
}

bool TtRssServiceRoot::onAfterMessagesDelete(RootItem *selected_item, const QList<Message> &messages) {
  Q_UNUSED(messages)

  // User deleted some messages he selected in message list.
  selected_item->updateCounts(true);

  if (selected_item->kind() == RootItemKind::Bin) {
    itemChanged(QList<RootItem*>() << m_recycleBin);
  }
  else {
    m_recycleBin->updateCounts(true);
    itemChanged(QList<RootItem*>() << selected_item << m_recycleBin);
  }

  requestFeedReadFilterReload();
  return true;
}

bool TtRssServiceRoot::onBeforeMessagesRestoredFromBin(RootItem *selected_item, const QList<Message> &messages) {
  return false;
}

bool TtRssServiceRoot::onAfterMessagesRestoredFromBin(RootItem *selected_item, const QList<Message> &messages) {
  return false;
}

TtRssNetworkFactory *TtRssServiceRoot::network() const {
  return m_network;
}

QStringList TtRssServiceRoot::customIDSOfMessagesForItem(RootItem *item) {
  if (item->getParentServiceRoot() != this) {
    // Not item from this account.
    return QStringList();
  }
  else {
    QStringList list;

    switch (item->kind()) {
      case RootItemKind::Category: {
        foreach (RootItem *child, item->childItems()) {
          list.append(customIDSOfMessagesForItem(child));
        }

        return list;
      }

      case RootItemKind::ServiceRoot: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        QSqlQuery query(database);

        query.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;"));
        query.bindValue(QSL(":account_id"), accountId());
        query.exec();

        while (query.next()) {
          list.append(query.value(0).toString());
        }

        break;
      }

      case RootItemKind::Bin: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        QSqlQuery query(database);

        query.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;"));
        query.bindValue(QSL(":account_id"), accountId());
        query.exec();

        while (query.next()) {
          list.append(query.value(0).toString());
        }

        break;
      }

      case RootItemKind::Feed: {
        QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
        QSqlQuery query(database);

        query.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;"));
        query.bindValue(QSL(":account_id"), accountId());
        query.bindValue(QSL(":feed"), qobject_cast<TtRssFeed*>(item)->customId());
        query.exec();

        while (query.next()) {
          list.append(query.value(0).toString());
        }

        break;
      }

      default:
        break;
    }

    return list;
  }
}

bool TtRssServiceRoot::markFeedsReadUnread(QList<Feed*> items, RootItem::ReadStatus read) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(db_handle);
  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare(QString("UPDATE Messages SET is_read = :read "
                                 "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0;").arg(textualFeedIds(items).join(QSL(", "))));

  query_read_msg.bindValue(QSL(":read"), read == RootItem::Read ? 1 : 0);

  if (query_read_msg.exec()) {
    QList<RootItem*> itemss;

    foreach (Feed *feed, items) {
      feed->updateCounts(false);
      itemss.append(feed);
    }

    itemChanged(itemss);
    requestReloadMessageList(read == RootItem::Read);
    return true;
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::cleanFeeds(QList<Feed*> items, bool clean_read_only) {
  QSqlDatabase db_handle = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_delete_msg(db_handle);
  query_delete_msg.setForwardOnly(true);

  if (clean_read_only) {
    if (!query_delete_msg.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                                          "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 1;").arg(textualFeedIds(items).join(QSL(", "))))) {
      qWarning("Query preparation failed for feeds clearing.");
      return false;
    }
  }
  else {
    if (!query_delete_msg.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                                          "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0;").arg(textualFeedIds(items).join(QSL(", "))))) {
      qWarning("Query preparation failed for feeds clearing.");
      return false;
    }
  }

  query_delete_msg.bindValue(QSL(":deleted"), 1);

  if (!query_delete_msg.exec()) {
    qDebug("Query execution for feeds clearing failed.");
    return false;
  }
  else {
    // Messages are cleared, now inform model about need to reload data.
    QList<RootItem*> itemss;

    foreach (Feed *feed, items) {
      feed->updateCounts(true);
      itemss.append(feed);
    }

    m_recycleBin->updateCounts(true);
    itemss.append(m_recycleBin);

    itemChanged(itemss);
    requestReloadMessageList(true);
    return true;
  }
}

void TtRssServiceRoot::saveAccountDataToDatabase() {
  if (accountId() != NO_PARENT_CATEGORY) {
    // We are overwritting previously saved data.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query(database);

    query.prepare("UPDATE TtRssAccounts "
                  "SET username = :username, password = :password, url = :url, auth_protected = :auth_protected, "
                  "auth_username = :auth_username, auth_password = :auth_password, force_update = :force_update "
                  "WHERE id = :id;");
    query.bindValue(QSL(":username"), m_network->username());
    query.bindValue(QSL(":password"), TextFactory::encrypt(m_network->password()));
    query.bindValue(QSL(":url"), m_network->url());
    query.bindValue(QSL(":auth_protected"), (int) m_network->authIsUsed());
    query.bindValue(QSL(":auth_username"), m_network->authUsername());
    query.bindValue(QSL(":auth_password"), TextFactory::encrypt(m_network->authPassword()));
    query.bindValue(QSL(":force_update"), (int) m_network->forceServerSideUpdate());
    query.bindValue(QSL(":id"), accountId());

    if (query.exec()) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
    else {
      qWarning("TT-RSS: Updating account failed: '%s'.", qPrintable(query.lastError().text()));
    }
  }
  else {
    // We are probably saving newly added account.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query(database);

    // First obtain the ID, which can be assigned to this new account.
    if (!query.exec("SELECT max(id) FROM Accounts;") || !query.next()) {
      qWarning("TT-RSS: Getting max ID from Accounts table failed: '%s'.", qPrintable(query.lastError().text()));
      return;
    }

    int id_to_assign = query.value(0).toInt() + 1;
    bool saved = true;

    query.prepare(QSL("INSERT INTO Accounts (id, type) VALUES (:id, :type);"));
    query.bindValue(QSL(":id"), id_to_assign);
    query.bindValue(QSL(":type"), SERVICE_CODE_TT_RSS);

    saved &= query.exec();

    query.prepare("INSERT INTO TtRssAccounts (id, username, password, auth_protected, auth_username, auth_password, url, force_update) "
                  "VALUES (:id, :username, :password, :auth_protected, :auth_username, :auth_password, :url, :force_update);");
    query.bindValue(QSL(":id"), id_to_assign);
    query.bindValue(QSL(":username"), m_network->username());
    query.bindValue(QSL(":password"), TextFactory::encrypt(m_network->password()));
    query.bindValue(QSL(":auth_protected"), (int) m_network->authIsUsed());
    query.bindValue(QSL(":auth_username"), m_network->authUsername());
    query.bindValue(QSL(":auth_password"), TextFactory::encrypt(m_network->authPassword()));
    query.bindValue(QSL(":url"), m_network->url());
    query.bindValue(QSL(":force_update"), (int) m_network->forceServerSideUpdate());

    saved &= query.exec();

    if (saved) {
      setId(id_to_assign);
      setAccountId(id_to_assign);
      updateTitle();
    }
    else {
      qWarning("TT-RSS: Saving of new account failed: '%s'.", qPrintable(query.lastError().text()));
    }
  }
}

void TtRssServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  Assignment categories;
  Assignment feeds;

  // Obtain data for categories from the database.
  QSqlQuery query_categories(database);
  query_categories.setForwardOnly(true);
  query_categories.prepare(QSL("SELECT * FROM Categories WHERE account_id = :account_id;"));
  query_categories.bindValue(QSL(":account_id"), accountId());

  if (!query_categories.exec()) {
    qFatal("Query for obtaining categories failed. Error message: '%s'.", qPrintable(query_categories.lastError().text()));
  }

  while (query_categories.next()) {
    AssignmentItem pair;
    pair.first = query_categories.value(CAT_DB_PARENT_ID_INDEX).toInt();
    pair.second = new TtRssCategory(query_categories.record());

    categories << pair;
  }

  // All categories are now loaded.
  QSqlQuery query_feeds(database);
  query_feeds.setForwardOnly(true);
  query_feeds.prepare(QSL("SELECT * FROM Feeds WHERE account_id = :account_id;"));
  query_feeds.bindValue(QSL(":account_id"), accountId());

  if (!query_feeds.exec()) {
    qFatal("Query for obtaining feeds failed. Error message: '%s'.", qPrintable(query_feeds.lastError().text()));
  }

  while (query_feeds.next()) {
    AssignmentItem pair;
    pair.first = query_feeds.value(FDS_DB_CATEGORY_INDEX).toInt();
    pair.second = new TtRssFeed(query_feeds.record());

    feeds << pair;
  }

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(m_recycleBin);
  m_recycleBin->updateCounts(true);
}

void TtRssServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->username() + QL1S("@") + host);
}

void TtRssServiceRoot::completelyRemoveAllData() {
  // Purge old data from SQL and clean all model items.
  removeOldFeedTree(true);
  cleanAllItems();
  updateCounts(true);
  itemChanged(QList<RootItem*>() << this);
  requestReloadMessageList(true);
}

void TtRssServiceRoot::syncIn() {
  QIcon original_icon = icon();

  setIcon(qApp->icons()->fromTheme(QSL("item-sync")));
  itemChanged(QList<RootItem*>() << this);

  TtRssGetFeedsCategoriesResponse feed_cats_response = m_network->getFeedsCategories();

  if (m_network->lastError() == QNetworkReply::NoError) {
    RootItem *new_tree = feed_cats_response.feedsCategories(true, m_network->url());

    // Purge old data from SQL and clean all model items.
    removeOldFeedTree(false);
    cleanAllItems();

    // Model is clean, now store new tree into DB and
    // set primary IDs of the items.
    storeNewFeedTree(new_tree);

    foreach (RootItem *top_level_item, new_tree->childItems()) {
      top_level_item->setParent(NULL);
      requestItemReassignment(top_level_item, this);
    }

    updateCounts(true);

    new_tree->clearChildren();
    new_tree->deleteLater();

    QList<RootItem*> all_items = getSubTree();

    itemChanged(all_items);
    requestReloadMessageList(true);
    requestItemExpand(all_items, true);
  }

  setIcon(original_icon);
  itemChanged(QList<RootItem*>() << this);
}

QStringList TtRssServiceRoot::customIDsOfMessages(const QList<QPair<Message,RootItem::Importance> > &changes) {
  QStringList list;

  for (int i = 0; i < changes.size(); i++) {
    list.append(changes.at(i).first.m_customId);
  }

  return list;
}

QStringList TtRssServiceRoot::customIDsOfMessages(const QList<Message> &messages) {
  QStringList list;

  foreach (const Message &message, messages) {
    list.append(message.m_customId);
  }

  return list;
}

QStringList TtRssServiceRoot::textualFeedIds(const QList<Feed*> &feeds) {
  QStringList stringy_ids;
  stringy_ids.reserve(feeds.size());

  foreach (Feed *feed, feeds) {
    stringy_ids.append(QString("'%1'").arg(QString::number(qobject_cast<TtRssFeed*>(feed)->customId())));
  }

  return stringy_ids;
}

void TtRssServiceRoot::removeOldFeedTree(bool including_messages) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query(database);
  query.setForwardOnly(true);

  query.prepare(QSL("DELETE FROM Feeds WHERE account_id = :account_id;"));
  query.bindValue(QSL(":account_id"), accountId());
  query.exec();

  query.prepare(QSL("DELETE FROM Categories WHERE account_id = :account_id;"));
  query.bindValue(QSL(":account_id"), accountId());
  query.exec();

  if (including_messages) {
    query.prepare(QSL("DELETE FROM Messages WHERE account_id = :account_id;"));
    query.bindValue(QSL(":account_id"), accountId());
    query.exec();
  }
}

void TtRssServiceRoot::cleanAllItems() {
  foreach (RootItem *top_level_item, childItems()) {
    if (top_level_item->kind() != RootItemKind::Bin) {
      requestItemRemoval(top_level_item);
    }
  }
}

void TtRssServiceRoot::storeNewFeedTree(RootItem *root) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_category(database);
  QSqlQuery query_feed(database);

  query_category.prepare("INSERT INTO Categories (parent_id, title, account_id, custom_id) "
                         "VALUES (:parent_id, :title, :account_id, :custom_id);");
  query_feed.prepare("INSERT INTO Feeds (title, icon, category, protected, update_type, update_interval, account_id, custom_id) "
                     "VALUES (:title, :icon, :category, :protected, :update_type, :update_interval, :account_id, :custom_id);");

  // Iterate all children.
  foreach (RootItem *child, root->getSubTree()) {
    if (child->kind() == RootItemKind::Category) {
      query_category.bindValue(QSL(":parent_id"), child->parent()->id());
      query_category.bindValue(QSL(":title"), child->title());
      query_category.bindValue(QSL(":account_id"), accountId());
      query_category.bindValue(QSL(":custom_id"), QString::number(qobject_cast<TtRssCategory*>(child)->customId()));

      if (query_category.exec()) {
        child->setId(query_category.lastInsertId().toInt());
      }
      else {
      }
    }
    else if (child->kind() == RootItemKind::Feed) {
      TtRssFeed *feed = static_cast<TtRssFeed*>(child);

      query_feed.bindValue(QSL(":title"), feed->title());
      query_feed.bindValue(QSL(":icon"), qApp->icons()->toByteArray(feed->icon()));
      query_feed.bindValue(QSL(":category"), feed->parent()->id());
      query_feed.bindValue(QSL(":protected"), 0);
      query_feed.bindValue(QSL(":update_type"), (int) feed->autoUpdateType());
      query_feed.bindValue(QSL(":update_interval"), feed->autoUpdateInitialInterval());
      query_feed.bindValue(QSL(":account_id"), accountId());
      query_feed.bindValue(QSL(":custom_id"), feed->customId());

      if (query_feed.exec()) {
        feed->setId(query_feed.lastInsertId().toInt());
      }
      else {
      }
    }
  }

  if (!childItems().contains(m_recycleBin)) {
    // As the last item, add recycle bin, which is needed.
    appendChild(m_recycleBin);
    m_recycleBin->updateCounts(true);
  }
}
