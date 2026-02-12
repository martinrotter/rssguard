// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "miscellaneous/application.h"
#include "services/abstract/gui/formaddeditlabel.h"
#include "services/abstract/serviceroot.h"

#include <QPainter>
#include <QPainterPath>

Label::Label(const QString& name, const QIcon& icon, RootItem* parent_item) : Label(parent_item) {
  setIcon(icon);
  setTitle(name);
}

Label::Label(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Label);
}

int Label::countOfUnreadMessages() const {
  return m_unreadCount;
}

int Label::countOfAllMessages() const {
  return m_totalCount;
}

bool Label::canBeEdited() const {
  return Globals::hasFlag(account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Editing);
}

bool Label::canBeDeleted() const {
  return Globals::hasFlag(account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Deleting);
}

void Label::deleteItem() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteLabel(db, this);
  });

  account()->requestItemRemoval(this, false);
}

void Label::updateCounts() {
  int account_id = account()->accountId();
  auto ac = qApp->database()->worker()->read<ArticleCounts>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getMessageCountsForLabel(db, this, account_id);
  });

  setCountOfAllMessages(ac.m_total);
  setCountOfUnreadMessages(ac.m_unread);
}

void Label::assignToMessage(const Message& msg, bool reload_feeds_model) {
  account()->onBeforeLabelMessageAssignmentChanged({this}, {msg}, true);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::assignLabelToMessage(db, this, msg);
  });

  if (reload_feeds_model) {
    account()->onAfterLabelMessageAssignmentChanged({this}, {msg}, true);
  }
}

void Label::deassignFromMessage(const Message& msg, bool reload_feeds_model) {
  account()->onBeforeLabelMessageAssignmentChanged({this}, {msg}, false);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deassignLabelFromMessage(db, this, msg);
  });

  if (reload_feeds_model) {
    account()->onAfterLabelMessageAssignmentChanged({this}, {msg}, false);
  }
}

void Label::setCountOfAllMessages(int totalCount) {
  m_totalCount = totalCount;
}

void Label::setCountOfUnreadMessages(int unreadCount) {
  m_unreadCount = unreadCount;
}

void Label::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = account();

  service->onBeforeMessagesDelete(this, {});

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::cleanLabelledMessages(db, this, clear_only_read);
  });

  service->onAfterMessagesDelete(this, {});
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

void Label::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::markLabelledMessagesReadUnread(db, this, status);
  });

  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}
