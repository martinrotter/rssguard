// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/unreadnode.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

UnreadNode::UnreadNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Unread);
  setId(ID_UNREAD);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-unread")));

  setTitle(tr("Unread articles"));
  setDescription(tr("You can find all unread articles here."));
}

QList<Message> UnreadNode::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedUnreadMessages(database, account()->accountId());
}

void UnreadNode::updateCounts(bool including_total_count) {
  Q_UNUSED(including_total_count)

  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = account()->accountId();

  m_totalCount = m_unreadCount = DatabaseQueries::getUnreadMessageCounts(database, account_id);
}

bool UnreadNode::cleanMessages(bool clean_read_only) {
  if (clean_read_only) {
    return true;
  }

  ServiceRoot* service = account();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::cleanUnreadMessages(database, service->accountId())) {
    service->updateCounts(true);
    service->itemChanged(service->getSubTree<RootItem>());
    service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
    return true;
  }
  else {
    return false;
  }
}

bool UnreadNode::markAsReadUnread(RootItem::ReadStatus status) {
  if (status == RootItem::ReadStatus::Unread) {
    // NOTE: We do not need to mark already unread messages as unread.
    return true;
  }

  ServiceRoot* service = account();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::markUnreadMessagesRead(database, service->accountId())) {
    service->updateCounts(false);
    service->itemChanged(service->getSubTree<RootItem>());
    service->informOthersAboutDataChange(this,
                                         status == RootItem::ReadStatus::Read
                                           ? FeedsModel::ExternalDataChange::MarkedRead
                                           : FeedsModel::ExternalDataChange::MarkedUnread);
    return true;
  }
  else {
    return false;
  }
}

int UnreadNode::countOfUnreadMessages() const {
  return m_unreadCount;
}

int UnreadNode::countOfAllMessages() const {
  return m_totalCount;
}
