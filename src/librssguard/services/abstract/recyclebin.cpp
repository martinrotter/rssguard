// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/recyclebin.h"

#include "database/databasequeries.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
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

void RecycleBin::updateCounts() {
  auto ac = qApp->database()->worker()->read<ArticleCounts>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getMessageCountsForBin(db, account()->accountId());
  });

  m_unreadCount = ac.m_unread;
  m_totalCount = ac.m_total;
}

void RecycleBin::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::markBinReadUnread(db, service->accountId(), status);
  });

  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}

void RecycleBin::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = account();

  service->onBeforeMessagesDelete(this, {});

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::cleanBin(db, clear_only_read, service->accountId());
  });

  service->onAfterMessagesDelete(this, {});
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

void RecycleBin::empty() {
  if (MsgBox::show({},
                   QMessageBox::Icon::Question,
                   tr("Are you sure?"),
                   tr("Do you really want to empty your recycle bin?"),
                   {},
                   {},
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::Yes,
                   QSL("clear_bin")) != QMessageBox::StandardButton::Yes) {
    return;
  }

  cleanMessages(false);
}

void RecycleBin::restore() {
  ServiceRoot* parent_root = account();

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::restoreBin(db, parent_root->accountId());
  });

  parent_root->updateCounts();
  parent_root->itemChanged(parent_root->getSubTree<RootItem>());
  parent_root->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::RecycleBinRestored);
}
