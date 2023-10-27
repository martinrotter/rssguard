// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/search.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/gui/formaddeditprobe.h"
#include "services/abstract/serviceroot.h"

#include <QPainter>
#include <QPainterPath>

Search::Search(const QString& name, const QString& filter, const QColor& color, RootItem* parent_item)
  : Search(parent_item) {
  setColor(color);
  setTitle(name);
  setFilter(filter);
}

Search::Search(RootItem* parent_item) : RootItem(parent_item), m_totalCount(-1), m_unreadCount(-1) {
  setKind(RootItem::Kind::Probe);
}

QColor Search::color() const {
  return m_color;
}

void Search::setColor(const QColor& color) {
  setIcon(IconFactory::generateIcon(color));
  m_color = color;
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

bool Search::deleteViaGui() {
  try {
    QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());
    DatabaseQueries::deleteProbe(db, this);
    getParentServiceRoot()->requestItemRemoval(this);
    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_CORE << "Failed to remove probe:" << QUOTE_W_SPACE_DOT(ex.message());
    return false;
  }
}

void Search::updateCounts(bool including_total_count) {
  Q_UNUSED(including_total_count)

  setCountOfAllMessages(-1);
  setCountOfUnreadMessages(-1);
}

QList<Message> Search::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesForProbe(database, this);
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

bool Search::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = getParentServiceRoot();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::cleanProbedMessages(database, clear_only_read, this);
    service->updateCounts(true);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(true);
    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_CORE << "Failed to clean messages of probe:" << QUOTE_W_SPACE_DOT(ex.message());
    return false;
  }
}

QString Search::additionalTooltip() const {
  return tr("Regular expression: %1").arg(QSL("<code>%1</code>").arg(filter()));
}

bool Search::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    try {
      cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_DB << "Cannot add some IDs to state cache:" << QUOTE_W_SPACE_DOT(ex.message());
      return false;
    }
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::markProbeReadUnread(database, this, status);
    service->updateCounts(false);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(status == RootItem::ReadStatus::Read);
    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Cannot mark probe as read/unread:" << QUOTE_W_SPACE_DOT(ex.message());
    return false;
  }
}
