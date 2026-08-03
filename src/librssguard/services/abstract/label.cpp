// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "exceptions/sqlexception.h"
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
  assignToMessages({msg}, reload_feeds_model);
}

void Label::deassignFromMessage(const Message& msg, bool reload_feeds_model) {
  deassignFromMessages({msg}, reload_feeds_model);
}

void Label::assignToMessages(const QList<Message>& messages, bool reload_feeds_model) {
  if (messages.isEmpty()) {
    return;
  }

  account()->onBeforeLabelMessageAssignmentChanged({this}, messages, true);

  DatabaseFactory* database = qApp->database();

  database->worker()->write([&](const QSqlDatabase& db) {
    QSqlDatabase transaction_db = db;

    if (!transaction_db.transaction()) {
      THROW_EX(SqlException, transaction_db.lastError());
    }

    try {
      DatabaseQueries::assignLabelToMessages(database->driver(), transaction_db, this, messages);

      if (!transaction_db.commit()) {
        THROW_EX(SqlException, transaction_db.lastError());
      }
    }
    catch (...) {
      transaction_db.rollback();
      throw;
    }
  });

  if (reload_feeds_model) {
    account()->onAfterLabelMessageAssignmentChanged({this}, messages, true);
  }
}

void Label::deassignFromMessages(const QList<Message>& messages, bool reload_feeds_model) {
  if (messages.isEmpty()) {
    return;
  }

  account()->onBeforeLabelMessageAssignmentChanged({this}, messages, false);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    QSqlDatabase transaction_db = db;

    if (!transaction_db.transaction()) {
      THROW_EX(SqlException, transaction_db.lastError());
    }

    try {
      DatabaseQueries::deassignLabelFromMessages(transaction_db, this, messages);

      if (!transaction_db.commit()) {
        THROW_EX(SqlException, transaction_db.lastError());
      }
    }
    catch (...) {
      transaction_db.rollback();
      throw;
    }
  });

  if (reload_feeds_model) {
    account()->onAfterLabelMessageAssignmentChanged({this}, messages, false);
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
  executeMessagesReadUnreadChange(status, [this, status](const QSqlDatabase& db) {
    DatabaseQueries::markLabelledMessagesReadUnread(db, this, status);
  });
}
