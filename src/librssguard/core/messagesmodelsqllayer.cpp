// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodelsqllayer.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

MessagesModelSqlLayer::MessagesModelSqlLayer() : m_filter(QSL(DEFAULT_SQL_MESSAGES_FILTER)) {
  m_db = qApp->database()->connection(QSL("MessagesModel"));

  // Used in <x>: SELECT <x1>, <x2> FROM ....;
  m_fieldNames[MSG_DB_ID_INDEX] = "Messages.id";
  m_fieldNames[MSG_DB_READ_INDEX] = "Messages.is_read";
  m_fieldNames[MSG_DB_DELETED_INDEX] = "Messages.is_deleted";
  m_fieldNames[MSG_DB_IMPORTANT_INDEX] = "Messages.is_important";
  m_fieldNames[MSG_DB_FEED_TITLE_INDEX] = "Feeds.title";
  m_fieldNames[MSG_DB_TITLE_INDEX] = "Messages.title";
  m_fieldNames[MSG_DB_URL_INDEX] = "Messages.url";
  m_fieldNames[MSG_DB_AUTHOR_INDEX] = "Messages.author";
  m_fieldNames[MSG_DB_DCREATED_INDEX] = "Messages.date_created";
  m_fieldNames[MSG_DB_CONTENTS_INDEX] = "Messages.contents";
  m_fieldNames[MSG_DB_PDELETED_INDEX] = "Messages.is_pdeleted";
  m_fieldNames[MSG_DB_ENCLOSURES_INDEX] = "Messages.enclosures";
  m_fieldNames[MSG_DB_ACCOUNT_ID_INDEX] = "Messages.account_id";
  m_fieldNames[MSG_DB_CUSTOM_ID_INDEX] = "Messages.custom_id";
  m_fieldNames[MSG_DB_CUSTOM_HASH_INDEX] = "Messages.custom_hash";
  m_fieldNames[MSG_DB_FEED_CUSTOM_ID_INDEX] = "Messages.feed";
  m_fieldNames[MSG_DB_HAS_ENCLOSURES] = "CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures";

  // Used in <x>: SELECT ... FROM ... ORDER BY <x1> DESC, <x2> ASC;
  m_orderByNames[MSG_DB_ID_INDEX] = "Messages.id";
  m_orderByNames[MSG_DB_READ_INDEX] = "Messages.is_read";
  m_orderByNames[MSG_DB_DELETED_INDEX] = "Messages.is_deleted";
  m_orderByNames[MSG_DB_IMPORTANT_INDEX] = "Messages.is_important";
  m_orderByNames[MSG_DB_FEED_TITLE_INDEX] = "Feeds.title";
  m_orderByNames[MSG_DB_TITLE_INDEX] = "Messages.title";
  m_orderByNames[MSG_DB_URL_INDEX] = "Messages.url";
  m_orderByNames[MSG_DB_AUTHOR_INDEX] = "Messages.author";
  m_orderByNames[MSG_DB_DCREATED_INDEX] = "Messages.date_created";
  m_orderByNames[MSG_DB_CONTENTS_INDEX] = "Messages.contents";
  m_orderByNames[MSG_DB_PDELETED_INDEX] = "Messages.is_pdeleted";
  m_orderByNames[MSG_DB_ENCLOSURES_INDEX] = "Messages.enclosures";
  m_orderByNames[MSG_DB_ACCOUNT_ID_INDEX] = "Messages.account_id";
  m_orderByNames[MSG_DB_CUSTOM_ID_INDEX] = "Messages.custom_id";
  m_orderByNames[MSG_DB_CUSTOM_HASH_INDEX] = "Messages.custom_hash";
  m_orderByNames[MSG_DB_FEED_CUSTOM_ID_INDEX] = "Messages.feed";
  m_orderByNames[MSG_DB_HAS_ENCLOSURES] = "has_enclosures";
}

void MessagesModelSqlLayer::addSortState(int column, Qt::SortOrder order) {
  int existing = m_sortColumns.indexOf(column);
  bool is_ctrl_pressed = (QApplication::queryKeyboardModifiers() & Qt::ControlModifier) == Qt::ControlModifier;

  if (existing >= 0) {
    m_sortColumns.removeAt(existing);
    m_sortOrders.removeAt(existing);
  }

  if (m_sortColumns.size() >= MAX_MULTICOLUMN_SORT_STATES) {
    // We support only limited number of sort states
    // due to DB performance.
    m_sortColumns.removeAt(0);
    m_sortOrders.removeAt(0);
  }

  if (is_ctrl_pressed) {
    // User is activating the multicolumn sort mode.
    m_sortColumns.append(column);
    m_sortOrders.append(order);
  }
  else {
    m_sortColumns.prepend(column);
    m_sortOrders.prepend(order);
  }
}

void MessagesModelSqlLayer::setFilter(const QString& filter) {
  m_filter = filter;
}

QString MessagesModelSqlLayer::formatFields() const {
  return m_fieldNames.values().join(QSL(", "));
}

QString MessagesModelSqlLayer::selectStatement() const {
  return QL1S("SELECT ") + formatFields() + QL1C(' ') +
         QL1S("FROM Messages LEFT JOIN Feeds ON Messages.feed = Feeds.custom_id AND Messages.account_id = Feeds.account_id "
              "WHERE ") +
         m_filter + orderByClause() + QL1C(';');
}

QString MessagesModelSqlLayer::orderByClause() const {
  if (m_sortColumns.isEmpty()) {
    return QString();
  }
  else {
    QStringList sorts;

    for (int i = 0; i < m_sortColumns.size(); i++) {
      QString field_name(m_orderByNames[m_sortColumns[i]]);

      sorts.append(field_name + (m_sortOrders[i] == Qt::AscendingOrder ? QSL(" ASC") : QSL(" DESC")));
    }

    return QL1S(" ORDER BY ") + sorts.join(QSL(", "));
  }
}
