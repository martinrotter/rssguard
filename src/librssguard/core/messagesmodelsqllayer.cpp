// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodelsqllayer.h"

#include "database/databasequeries.h"
#include "database/sqlquery.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "miscellaneous/application.h"

MessagesModelSqlLayer::MessagesModelSqlLayer() : m_filter(QSL(DEFAULT_SQL_MESSAGES_FILTER)) {
  m_db = qApp->database()->driver()->connection(QSL("MessagesModel"));

  // Used in <x>: SELECT <x1>, <x2> FROM ....;
  m_fieldNames = DatabaseQueries::messageTableAttributes();

  // Used in <x>: SELECT ... FROM ... ORDER BY <x1> DESC, <x2> ASC;
  m_orderByNames.append(QSL("Messages.id"));
  m_orderByNames.append(QSL("Messages.is_read"));
  m_orderByNames.append(QSL("Messages.is_important"));
  m_orderByNames.append(QSL("Messages.is_deleted"));
  m_orderByNames.append(QSL("Messages.is_pdeleted"));
  m_orderByNames.append(QSL("Messages.feed"));
  m_orderByNames.append(QSL("Messages.title"));
  m_orderByNames.append(QSL("Messages.url"));
  m_orderByNames.append(QSL("Messages.author"));
  m_orderByNames.append(QSL("Messages.date_created"));
  m_orderByNames.append(QSL("Messages.contents"));
  m_orderByNames.append(QSL("Messages.enclosures"));
  m_orderByNames.append(QSL("Messages.score"));
  m_orderByNames.append(QSL("Messages.account_id"));
  m_orderByNames.append(QSL("Messages.custom_id"));
  m_orderByNames.append(QSL("Messages.custom_data"));
  m_orderByNames.append(QSL("msg_labels"));
}

void MessagesModelSqlLayer::addSortState(int column, Qt::SortOrder order, bool ignore_multicolumn_sorting) {
  // We need to map model/header column to source database column.
  column = mapColumnToDatabase(column);

  if (column < 0) {
    qWarningNN << LOGSEC_MESSAGEMODEL << "Cannot sort this column.";
    return;
  }

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

    qDebugNN << LOGSEC_MESSAGEMODEL << "CTRL is pressed while sorting articles - sorting in backwards mode.";
  }
  else {
    m_sortColumns.prepend(column);
    m_sortOrders.prepend(order);

    qDebugNN << LOGSEC_MESSAGEMODEL << "CTRL is NOT pressed while sorting articles - sorting in standard mode.";
  }
}

void MessagesModelSqlLayer::clearSortStates() {
  m_sortColumns.clear();
  m_sortOrders.clear();
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

QList<Message> MessagesModelSqlLayer::fetchMessages(const QHash<QString, Label*>& labels,
                                                    int limit,
                                                    int offset,
                                                    int additional_article_id) const {
  QList<Message> msgs;

  if (limit > 0) {
    msgs.reserve(limit);
  }

  QString statemnt = selectStatement(limit, offset, additional_article_id);
  SqlQuery q(m_db);

  q.exec(statemnt);

  while (q.next()) {
    msgs.append(Message::fromSqlQuery(q, labels));
  }

  q.finish();

  return msgs;
}

QString MessagesModelSqlLayer::formatFields() const {
  return m_fieldNames.join(QSL(", "));
}

int MessagesModelSqlLayer::mapColumnToDatabase(int column) const {
  switch (column) {
    case MSG_MDL_ID_INDEX:
      return MSG_DB_ID_INDEX;

    case MSG_MDL_READ_INDEX:
      return MSG_DB_READ_INDEX;

    case MSG_MDL_IMPORTANT_INDEX:
      return MSG_DB_IMPORTANT_INDEX;

    case MSG_MDL_DELETED_INDEX:
      return MSG_DB_DELETED_INDEX;

    case MSG_MDL_PDELETED_INDEX:
      return MSG_DB_PDELETED_INDEX;

    case MSG_MDL_FEED_TITLE_INDEX:
    case MSG_MDL_FEED_ID_INDEX:
      return MSG_DB_FEED_ID_INDEX;

    case MSG_MDL_TITLE_INDEX:
      return MSG_DB_TITLE_INDEX;

    case MSG_MDL_URL_INDEX:
      return MSG_DB_URL_INDEX;

    case MSG_MDL_AUTHOR_INDEX:
      return MSG_DB_AUTHOR_INDEX;

    case MSG_MDL_DCREATED_INDEX:
      return MSG_DB_DCREATED_INDEX;

    case MSG_MDL_CONTENTS_INDEX:
      return MSG_DB_CONTENTS_INDEX;

    case MSG_MDL_SCORE_INDEX:
      return MSG_DB_SCORE_INDEX;

    case MSG_MDL_ACCOUNT_ID_INDEX:
      return MSG_DB_ACCOUNT_ID_INDEX;

    case MSG_MDL_CUSTOM_ID_INDEX:
      return MSG_DB_CUSTOM_ID_INDEX;

    case MSG_MDL_CUSTOM_DATA_INDEX:
      return MSG_DB_CUSTOM_DATA_INDEX;

    case MSG_MDL_LABELS:
      return MSG_DB_LABELS_IDS;

    case MSG_MDL_HAS_ENCLOSURES:
      return MSG_DB_ENCLOSURES_INDEX;

    default:
      return -1;
  }
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

      sorts.append(QSL("%1 %2").arg(field_name,
                                    m_sortOrders[i] == Qt::SortOrder::AscendingOrder ? QSL("ASC") : QSL("DESC")));
    }

    return QL1S("ORDER BY ") + sorts.join(QSL(", "));
  }
}

QString MessagesModelSqlLayer::limitOffset(int limit, int offset) const {
  return qApp->database()->driver()->limitOffset(limit, offset);
}
