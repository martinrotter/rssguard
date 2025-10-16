// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/recyclebin.h"

#include "database/databasequeries.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

RecycleBin::RecycleBin(RootItem* parent_item) : RootItem(parent_item), m_totalCount(0), m_unreadCount(0) {
  setKind(RootItem::Kind::Bin);
  setId(ID_RECYCLE_BIN);
  setIcon(qApp->icons()->fromTheme(QSL("user-trash")));
  setTitle(tr("Recycle bin"));
  setDescription(tr("Recycle bin contains all deleted articles from all feeds."));
}

QString RecycleBin::additionalTooltip() const {
  return tr("%n deleted article(s).", nullptr, countOfAllMessages());
}

int RecycleBin::countOfUnreadMessages() const {
  return m_unreadCount;
}

int RecycleBin::countOfAllMessages() const {
  return m_totalCount;
}

void RecycleBin::updateCounts(bool update_total_count) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  auto ac = DatabaseQueries::getMessageCountsForBin(database, account()->accountId());

  m_unreadCount = ac.m_unread;

  if (update_total_count) {
    m_totalCount = ac.m_total;
  }
}

QList<QAction*> RecycleBin::contextMenuFeedsList() {
  if (m_contextMenu.isEmpty()) {
    QAction* restore_action =
      new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Restore recycle bin"), this);
    QAction* empty_action = new QAction(qApp->icons()->fromTheme(QSL("edit-clear")), tr("Empty recycle bin"), this);

    connect(restore_action, &QAction::triggered, this, &RecycleBin::restore);
    connect(empty_action, &QAction::triggered, this, &RecycleBin::empty);

    m_contextMenu.append(restore_action);
    m_contextMenu.append(empty_action);
  }

  return m_contextMenu;
}

QList<Message> RecycleBin::undeletedMessages() const {
  const int account_id = account()->accountId();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesForBin(database, account_id);
}

bool RecycleBin::markAsReadUnread(RootItem::ReadStatus status) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  ServiceRoot* service = account();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  if (DatabaseQueries::markBinReadUnread(database, service->accountId(), status)) {
    updateCounts(false);
    service->itemChanged(QList<RootItem*>() << this);
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

bool RecycleBin::cleanMessages(bool clear_only_read) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  ServiceRoot* parent_root = account();

  if (DatabaseQueries::purgeMessagesFromBin(database, clear_only_read, parent_root->accountId())) {
    updateCounts(true);
    parent_root->itemChanged({this});
    parent_root->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
    return true;
  }
  else {
    return false;
  }
}

bool RecycleBin::empty() {
  if (MsgBox::show(nullptr,
                   QMessageBox::Icon::Question,
                   tr("Are you sure?"),
                   tr("Do you really want to empty your recycle bin?"),
                   {},
                   {},
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes) {
    return false;
  }

  return cleanMessages(false);
}

bool RecycleBin::restore() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  ServiceRoot* parent_root = account();

  if (DatabaseQueries::restoreBin(database, parent_root->accountId())) {
    parent_root->updateCounts(true);
    parent_root->itemChanged(parent_root->getSubTree<RootItem>());
    parent_root->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::RecycleBinRestored);
    return true;
  }
  else {
    return false;
  }
}
