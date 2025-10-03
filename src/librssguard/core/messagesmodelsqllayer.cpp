// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodelsqllayer.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "miscellaneous/application.h"

MessagesModelSqlLayer::MessagesModelSqlLayer()
  : m_filter(QSL(DEFAULT_SQL_MESSAGES_FILTER)), m_fieldNames({}), m_orderByNames({}), m_sortColumns({}),
    m_numericColumns({}), m_sortOrders({}) {
  m_db = qApp->database()->driver()->connection(QSL("MessagesModel"));

  // Used in <x>: SELECT <x1>, <x2> FROM ....;
  m_fieldNames = DatabaseQueries::messageTableAttributes(m_db.driverName() == QSL(APP_DB_SQLITE_DRIVER));

  // Used in <x>: SELECT ... FROM ... ORDER BY <x1> DESC, <x2> ASC;
  m_orderByNames[MSG_DB_ID_INDEX] = QSL("Messages.id");
  m_orderByNames[MSG_DB_READ_INDEX] = QSL("Messages.is_read");
  m_orderByNames[MSG_DB_IMPORTANT_INDEX] = QSL("Messages.is_important");
  m_orderByNames[MSG_DB_DELETED_INDEX] = QSL("Messages.is_deleted");
  m_orderByNames[MSG_DB_PDELETED_INDEX] = QSL("Messages.is_pdeleted");
  m_orderByNames[MSG_DB_FEED_CUSTOM_ID_INDEX] = QSL("Messages.feed");
  m_orderByNames[MSG_DB_TITLE_INDEX] = QSL("Messages.title");
  m_orderByNames[MSG_DB_URL_INDEX] = QSL("Messages.url");
  m_orderByNames[MSG_DB_AUTHOR_INDEX] = QSL("Messages.author");
  m_orderByNames[MSG_DB_DCREATED_INDEX] = QSL("Messages.date_created");
  m_orderByNames[MSG_DB_CONTENTS_INDEX] = QSL("Messages.contents");
  m_orderByNames[MSG_DB_ENCLOSURES_INDEX] = QSL("Messages.enclosures");
  m_orderByNames[MSG_DB_SCORE_INDEX] = QSL("Messages.score");
  m_orderByNames[MSG_DB_ACCOUNT_ID_INDEX] = QSL("Messages.account_id");
  m_orderByNames[MSG_DB_CUSTOM_ID_INDEX] = QSL("Messages.custom_id");
  m_orderByNames[MSG_DB_CUSTOM_HASH_INDEX] = QSL("Messages.custom_hash");
  m_orderByNames[MSG_DB_FEED_TITLE_INDEX] = QSL("Messages.feed");
  m_orderByNames[MSG_DB_FEED_IS_RTL_INDEX] = QSL("rtl");
  m_orderByNames[MSG_DB_HAS_ENCLOSURES] = QSL("has_enclosures");
  m_orderByNames[MSG_DB_LABELS] = QSL("msg_labels");
  m_orderByNames[MSG_DB_LABELS_IDS] = QSL("Messages.labels");

  m_numericColumns << MSG_DB_ID_INDEX << MSG_DB_READ_INDEX << MSG_DB_DELETED_INDEX << MSG_DB_PDELETED_INDEX
                   << MSG_DB_IMPORTANT_INDEX << MSG_DB_ACCOUNT_ID_INDEX << MSG_DB_DCREATED_INDEX << MSG_DB_SCORE_INDEX
                   << MSG_DB_FEED_IS_RTL_INDEX;
}

void MessagesModelSqlLayer::addSortState(int column, Qt::SortOrder order, bool ignore_multicolumn_sorting) {
  int existing = m_sortColumns.indexOf(column);
  bool is_ctrl_pressed =
    Globals::hasFlag(QApplication::queryKeyboardModifiers(), Qt::KeyboardModifier::ControlModifier);

  if (existing >= 0) {
    m_sortColumns.removeAt(existing);
    m_sortOrders.removeAt(existing);
  }

  if (m_sortColumns.size() >= MAX_MULTICOLUMN_SORT_STATES) {
    // We support only limited number of sort states
    // due to DB performance.
    m_sortColumns.removeLast();
    m_sortOrders.removeLast();
  }

  if (is_ctrl_pressed && !ignore_multicolumn_sorting) {
    // User is activating the multicolumn sort mode.
    m_sortColumns.append(column);
    m_sortOrders.append(order);

    qDebugNN << LOGSEC_MESSAGEMODEL << "CTRL is pressed while sorting articles - sorting with multicolumn mode.";
  }
  else {
    m_sortColumns.prepend(column);
    m_sortOrders.prepend(order);

    qDebugNN << LOGSEC_MESSAGEMODEL << "CTRL is NOT pressed while sorting articles - sorting with standard mode.";
  }
}

void MessagesModelSqlLayer::setFilter(const QString& filter) {
  m_filter = filter;
}

SortColumnsAndOrders MessagesModelSqlLayer::sortColumnAndOrders() const {
  SortColumnsAndOrders res;

  res.m_columns = m_sortColumns;
  res.m_orders = m_sortOrders;

  return res;
}

QList<Message> MessagesModelSqlLayer::fetchMessages(int limit, int offset, int additional_article_id) const {
  QList<Message> msgs;

  if (limit > 0) {
    msgs.reserve(limit);
  }

  QString statemnt = selectStatement(limit, offset, additional_article_id);
  QSqlQuery q(m_db);

  q.setForwardOnly(true);

  if (!q.exec(statemnt)) {
    throw ApplicationException(q.lastError().text());
  }

  PRINT_QUERY(q)

  while (q.next()) {
    msgs.append(Message::fromSqlQuery(q));
  }

  q.finish();

  return msgs;
}

QString MessagesModelSqlLayer::formatFields() const {
  return m_fieldNames.values().join(QSL(", "));
}

bool MessagesModelSqlLayer::isColumnNumeric(int column_id) const {
  return m_numericColumns.contains(column_id);
}

QString MessagesModelSqlLayer::selectStatement(int limit, int offset, int additional_article_id) const {
  QString fltr;

  if (additional_article_id <= 0) {
    fltr = m_filter;
  }
  else {
    fltr = QSL("(%1) OR Messages.id = %2").arg(m_filter, QString::number(additional_article_id));
  }

  return QL1S("SELECT ") + formatFields() + QL1S(" FROM Messages WHERE ") + fltr + QL1S(" ") + orderByClause() +
         QL1S(" ") + limitOffset(limit, offset) + QL1C(';');
}

QString MessagesModelSqlLayer::orderByClause() const {
  if (m_sortColumns.isEmpty()) {
    return QString();
  }
  else {
    QStringList sorts;

    for (int i = 0; i < m_sortColumns.size(); i++) {
      QString field_name(m_orderByNames[m_sortColumns[i]]);
      // TODO: nepoužít lower ale místo toho na úrovni DB
      // mariadb -> specifikovat colate při tvorbě tabulek
      // sqlite -> nocase colate taktéž při tvorbě tabulek
      QString order_sql = isColumnNumeric(m_sortColumns[i]) ? QSL("%1") : QSL("LOWER(%1)");

      sorts.append(order_sql.arg(field_name) +
                   (m_sortOrders[i] == Qt::SortOrder::AscendingOrder ? QSL(" ASC") : QSL(" DESC")));
    }

    return QL1S("ORDER BY ") + sorts.join(QSL(", "));
  }
}

QString MessagesModelSqlLayer::limitOffset(int limit, int offset) const {
  return qApp->database()->driver()->limitOffset(limit, offset);
}
