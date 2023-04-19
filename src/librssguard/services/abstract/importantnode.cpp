// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/importantnode.h"

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

ImportantNode::ImportantNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Important);
  setId(ID_IMPORTANT);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-important")));
  setTitle(tr("Important articles"));
  setDescription(tr("You can find all important articles here."));
}

QList<Message> ImportantNode::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedImportantMessages(database, getParentServiceRoot()->accountId());
}

void ImportantNode::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = getParentServiceRoot()->accountId();

  if (including_total_count) {
    m_totalCount = DatabaseQueries::getImportantMessageCounts(database, account_id, true);
  }

  m_unreadCount = DatabaseQueries::getImportantMessageCounts(database, account_id, false);
}

bool ImportantNode::cleanMessages(bool clean_read_only) {
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

bool ImportantNode::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
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

int ImportantNode::countOfUnreadMessages() const {
  return m_unreadCount;
}

int ImportantNode::countOfAllMessages() const {
  return m_totalCount;
}
