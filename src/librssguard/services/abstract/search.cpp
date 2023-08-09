// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/search.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/gui/formaddeditprobe.h"
#include "services/abstract/labelsnode.h"
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
  setIcon(generateIcon(color));
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

bool Search::editViaGui() {
  FormAddEditProbe form(qApp->mainFormWidget());

  if (form.execForEdit(this)) {
    QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

    try {
      DatabaseQueries::updateProbe(db, this);
      return true;
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_CORE << "Failed to edit probe:" << QUOTE_W_SPACE_DOT(ex.message());
      return false;
    }
  }
  else {
    return true;
  }
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
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = getParentServiceRoot()->accountId();

  /*
  auto ac = DatabaseQueries::getMessageCountsForLabel(database, this, account_id);

  if (including_total_count) {
    setCountOfAllMessages(ac.m_total);
  }

  setCountOfUnreadMessages(ac.m_unread);
  */
}

QList<Message> Search::undeletedMessages() const {
  return {};
  /*
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesWithLabel(database, this);
  */
}

QIcon Search::generateIcon(const QColor& color) {
  QPixmap pxm(64, 64);

  pxm.fill(Qt::GlobalColor::transparent);

  QPainter paint(&pxm);

  paint.setBrush(color);
  paint.setPen(Qt::GlobalColor::transparent);
  paint.drawEllipse(pxm.rect().marginsRemoved(QMargins(2, 2, 2, 2)));

  return pxm;
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

  return false;
  /*
  if (DatabaseQueries::cleanLabelledMessages(database, clear_only_read, this)) {
    service->updateCounts(true);
    service->itemChanged(service->getSubTree());
    service->requestReloadMessageList(true);
    return true;
  }
  else {
    return false;
  }
  */
}

bool Search::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  /*
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
  */

  return false;
}
