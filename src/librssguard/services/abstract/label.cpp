// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "gui/dialogs/formaddeditlabel.h"
#include "miscellaneous/application.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/labelsnode.h"
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
  setIcon(generateIcon(color));
  m_color = color;
}

int Label::countOfUnreadMessages() const {
  return m_unreadCount;
}

int Label::countOfAllMessages() const {
  return m_totalCount;
}

bool Label::canBeEdited() const {
  return (getParentServiceRoot()->supportedLabelOperations() & ServiceRoot::LabelOperation::Editing) ==
         ServiceRoot::LabelOperation::Editing;
}

bool Label::editViaGui() {
  FormAddEditLabel form(qApp->mainFormWidget());

  if (form.execForEdit(this)) {
    QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

    return DatabaseQueries::updateLabel(db, this);
  }
  else {
    return false;
  }
}

bool Label::canBeDeleted() const {
  return (getParentServiceRoot()->supportedLabelOperations() & ServiceRoot::LabelOperation::Deleting) ==
         ServiceRoot::LabelOperation::Deleting;
}

bool Label::deleteViaGui() {
  QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::deleteLabel(db, this)) {
    getParentServiceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

void Label::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = getParentServiceRoot()->accountId();

  if (including_total_count) {
    setCountOfAllMessages(DatabaseQueries::getMessageCountsForLabel(database, this, account_id, true));
  }

  setCountOfUnreadMessages(DatabaseQueries::getMessageCountsForLabel(database, this, account_id, false));
}

QList<Message> Label::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesWithLabel(database, this);
}

QIcon Label::generateIcon(const QColor& color) {
  QPixmap pxm(64, 64);

  pxm.fill(Qt::GlobalColor::transparent);

  QPainter paint(&pxm);

  paint.setBrush(color);
  paint.setPen(Qt::GlobalColor::transparent);
  paint.drawEllipse(pxm.rect().marginsRemoved(QMargins(2, 2, 2, 2)));

  return pxm;
}

void Label::assignToMessage(const Message& msg) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

  if (getParentServiceRoot()->onBeforeLabelMessageAssignmentChanged({this}, {msg}, true)) {
    DatabaseQueries::assignLabelToMessage(database, this, msg);

    getParentServiceRoot()->onAfterLabelMessageAssignmentChanged({this}, {msg}, true);
  }
}

void Label::deassignFromMessage(const Message& msg) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

  if (getParentServiceRoot()->onBeforeLabelMessageAssignmentChanged({this}, {msg}, false)) {
    DatabaseQueries::deassignLabelFromMessage(database, this, msg);

    getParentServiceRoot()->onAfterLabelMessageAssignmentChanged({this}, {msg}, false);
  }
}

void Label::setCountOfAllMessages(int totalCount) {
  m_totalCount = totalCount;
}

void Label::setCountOfUnreadMessages(int unreadCount) {
  m_unreadCount = unreadCount;
}

bool Label::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = getParentServiceRoot();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::cleanLabelledMessages(database, clear_only_read, this)) {
    service->updateCounts(true);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(true);
    return true;
  }
  else {
    return false;
  }
}

bool Label::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  if (DatabaseQueries::markLabelledMessagesReadUnread(database, this, status)) {
    service->updateCounts(false);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(status == RootItem::ReadStatus::Read);
    return true;
  }
  else {
    return false;
  }
}
