// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/importantnode.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

#include <QThread>

ImportantNode::ImportantNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItemKind::Important);
  setId(ID_RECYCLE_BIN);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-important")));
  setTitle(tr("Important messages"));
  setDescription(tr("You can find all important messages here."));
  setCreationDate(QDateTime::currentDateTime());
}

void ImportantNode::updateCounts(bool including_total_count) {
  bool is_main_thread = QThread::currentThread() == qApp->thread();
  QSqlDatabase database = is_main_thread ?
                          qApp->database()->connection(metaObject()->className()) :
                          qApp->database()->connection(QSL("feed_upd"));
  int account_id = getParentServiceRoot()->accountId();

  if (including_total_count) {
    m_totalCount = DatabaseQueries::getImportantMessageCounts(database, account_id, true);
  }

  m_unreadCount = DatabaseQueries::getImportantMessageCounts(database, account_id, false);
}

int ImportantNode::countOfUnreadMessages() const {
  return m_unreadCount;
}

int ImportantNode::countOfAllMessages() const {
  return m_totalCount;
}
