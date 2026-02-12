// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/search.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "services/abstract/gui/formaddeditprobe.h"
#include "services/abstract/serviceroot.h"

#include <QPainter>
#include <QPainterPath>

Search::Search(const QString& name, Type type, const QString& filter, const QIcon& icon, RootItem* parent_item)
  : Search(parent_item) {
  setType(type);
  setIcon(icon);
  setTitle(name);
  setFilter(filter);
}

Search::Search(RootItem* parent_item)
  : RootItem(parent_item), m_type(Type::Regex), m_totalCount(-1), m_unreadCount(-1) {
  setKind(RootItem::Kind::Probe);
}

int Search::countOfUnreadMessages() const {
  return m_unreadCount;
}

int Search::countOfAllMessages() const {
  return m_totalCount;
}

bool Search::canBeEdited() const {
  return true;
}

bool Search::canBeDeleted() const {
  return true;
}

void Search::deleteItem() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteProbe(db, this);
  });

  account()->requestItemRemoval(this, false);
}

void Search::updateCounts() {
  setCountOfAllMessages(-1);
  setCountOfUnreadMessages(-1);
}

Search::Type Search::type() const {
  return m_type;
}

void Search::setType(Type type) {
  m_type = type;
}

QString Search::filter() const {
  return m_filter;
}

void Search::setFilter(const QString& new_filter) {
  m_filter = new_filter;
}

void Search::setCountOfAllMessages(int totalCount) {
  m_totalCount = totalCount;
}

void Search::setCountOfUnreadMessages(int unreadCount) {
  m_unreadCount = unreadCount;
}

QString Search::additionalTooltip() const {
  if (m_type == Type::Regex) {
    return tr("Regular expression: %1").arg(QSL("<code>%1</code>").arg(filter()));
  }
  else {
    return tr("SQL 'WHERE' clause: %1").arg(QSL("<code>%1</code>").arg(filter()));
  }
}

void Search::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = account();

  service->onBeforeMessagesDelete(this, {});

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::cleanProbedMessages(db, this, clear_only_read);
  });

  service->onAfterMessagesDelete(this, {});
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

void Search::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::markProbeReadUnread(db, this, status);
  });

  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}
