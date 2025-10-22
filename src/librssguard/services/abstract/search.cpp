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

void Search::deleteItem() {
  QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());
  DatabaseQueries::deleteProbe(db, this);
  account()->requestItemRemoval(this);
}

void Search::updateCounts(bool including_total_count) {
  Q_UNUSED(including_total_count)

  setCountOfAllMessages(-1);
  setCountOfUnreadMessages(-1);
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

void Search::cleanMessages(bool clear_only_read) {
  ServiceRoot* service = account();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::cleanProbedMessages(database, clear_only_read, this);
  service->updateCounts(true);
  service->itemChanged(service->getSubTree<RootItem>());
  service->informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
}

QString Search::additionalTooltip() const {
  return tr("Regular expression: %1").arg(QSL("<code>%1</code>").arg(filter()));
}

void Search::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::markProbeReadUnread(database, this, status);
  service->updateCounts(false);
  service->itemChanged(service->getSubTree<RootItem>());
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}
