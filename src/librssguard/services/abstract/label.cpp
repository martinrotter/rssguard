// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "miscellaneous/application.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/gui/formaddeditlabel.h"
#include "services/abstract/serviceroot.h"

#include <QPainter>
#include <QPainterPath>

Label::Label(const QString& name, const QColor& color, RootItem* parent_item) : Label(parent_item) {
  setColor(color);
  setTitle(name);
}

Label::Label(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Label);
}

QColor Label::color() const {
  return m_color;
}

void Label::setColor(const QColor& color) {
  setIcon(IconFactory::generateIcon(color));
  m_color = color;
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

bool Label::deleteItem() {
  QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::deleteLabel(db, this)) {
    account()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

void Label::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = account()->accountId();
  auto ac = DatabaseQueries::getMessageCountsForLabel(database, this, account_id);

  if (including_total_count) {
    setCountOfAllMessages(ac.m_total);
  }

  setCountOfUnreadMessages(ac.m_unread);
}

QList<Message> Label::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesWithLabel(database, this);
}

void Label::assignToMessage(const Message& msg, bool reload_feeds_model) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

  if (account()->onBeforeLabelMessageAssignmentChanged({this}, {msg}, true)) {
    DatabaseQueries::assignLabelToMessage(database, this, msg);

    if (reload_feeds_model) {
      account()->onAfterLabelMessageAssignmentChanged({this}, {msg}, true);
    }
  }
}

void Label::deassignFromMessage(const Message& msg, bool reload_feeds_model) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

  if (account()->onBeforeLabelMessageAssignmentChanged({this}, {msg}, false)) {
    DatabaseQueries::deassignLabelFromMessage(database, this, msg);

    if (reload_feeds_model) {
      account()->onAfterLabelMessageAssignmentChanged({this}, {msg}, false);
    }
  }
}

void Label::setCountOfAllMessages(int totalCount) {
  m_totalCount = totalCount;
}

void Label::setCountOfUnreadMessages(int unreadCount) {
  m_unreadCount = unreadCount;
}

bool Label::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = account();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::cleanLabelledMessages(database, clear_only_read, this)) {
    service->updateCounts(true);
    service->itemChanged(service->getSubTree<RootItem>());
    service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
    return true;
  }
  else {
    return false;
  }
}

bool Label::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::markLabelledMessagesReadUnread(database, this, status)) {
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
