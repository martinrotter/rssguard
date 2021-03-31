// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/unreadnode.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QThread>

UnreadNode::UnreadNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Unread);
  setId(ID_UNREAD);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-unread")));
  setTitle(tr("Unread messages"));
  setDescription(tr("You can find all unread messages here."));
}

QList<Message> UnreadNode::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedUnreadMessages(database, getParentServiceRoot()->accountId());
}

void UnreadNode::updateCounts(bool including_total_count) {
  Q_UNUSED(including_total_count)

  bool is_main_thread = QThread::currentThread() == qApp->thread();
  QSqlDatabase database = is_main_thread ?
                          qApp->database()->driver()->connection(metaObject()->className()) :
                          qApp->database()->driver()->connection(QSL("feed_upd"));
  int account_id = getParentServiceRoot()->accountId();

  m_totalCount = m_unreadCount = DatabaseQueries::getUnreadMessageCounts(database, account_id);
}

bool UnreadNode::cleanMessages(bool clean_read_only) {
  ServiceRoot* service = getParentServiceRoot();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::cleanImportantMessages(database, clean_read_only, service->accountId())) {
    service->updateCounts(true);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(true);
    return true;
  }
  else {
    return false;
  }
}

bool UnreadNode::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this), status);
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::markImportantMessagesReadUnread(database, service->accountId(), status)) {
    service->updateCounts(false);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(status == RootItem::ReadStatus::Read);
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
