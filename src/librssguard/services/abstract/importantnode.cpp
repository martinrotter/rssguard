// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/importantnode.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

ImportantNode::ImportantNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Important);
  setId(ID_IMPORTANT);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-important")));
  setTitle(tr("Important articles"));
  setDescription(tr("You can find all important articles here."));
}

void ImportantNode::updateCounts() {
  int account_id = account()->accountId();
  auto ac = qApp->database()->worker()->read<ArticleCounts>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getImportantMessageCounts(db, account_id);
  });

  m_totalCount = ac.m_total;
  m_unreadCount = ac.m_unread;
}

void ImportantNode::cleanMessages(bool clean_read_only) {
  ServiceRoot* service = account();

  service->onBeforeMessagesDelete(this, {});

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::cleanImportantMessages(db, clean_read_only, service->accountId());
  });

  service->onAfterMessagesDelete(this, {});
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

void ImportantNode::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::markImportantMessagesReadUnread(db, service->accountId(), status);
  });

  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}

int ImportantNode::countOfUnreadMessages() const {
  return m_unreadCount;
}

int ImportantNode::countOfAllMessages() const {
  return m_totalCount;
}
