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

void UnreadNode::updateCounts() {
  int account_id = account()->accountId();

  m_totalCount = m_unreadCount = qApp->database()->worker()->read<int>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getUnreadMessageCounts(db, account_id).m_total;
  });
}

void UnreadNode::cleanMessages(bool clean_read_only) {
  if (clean_read_only) {
    return;
  }

  ServiceRoot* service = account();

  service->onBeforeMessagesDelete(this, {});

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::cleanUnreadMessages(db, service->accountId());
  });

  service->onAfterMessagesDelete(this, {});
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

void UnreadNode::markAsReadUnread(RootItem::ReadStatus status) {
  if (status == RootItem::ReadStatus::Unread) {
    // NOTE: We do not need to mark already unread messages as unread.
    return;
  }

  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::markUnreadMessagesRead(db, service->accountId());
  });

  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}

int UnreadNode::countOfUnreadMessages() const {
  return m_unreadCount;
}

int UnreadNode::countOfAllMessages() const {
  return m_totalCount;
}
