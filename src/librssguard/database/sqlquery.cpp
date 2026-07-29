// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/sqlquery.h"

#include "definitions/definitions.h"
#include "exceptions/sqlexception.h"

SqlQuery::SqlQuery(const QSqlDatabase& db) : QSqlQuery(db) {
  setForwardOnly(true);
}

bool SqlQuery::exec(const QString& query, bool throw_ex) {
  const bool ok = QSqlQuery::exec(query);

  logQuery();

  if (!ok && throw_ex) {
    qCriticalNN << LOGSEC_DB << "SQL exception:" << QUOTE_W_SPACE_DOT(lastError().text());
    THROW_EX(SqlException, lastError());
  }

  return ok;
}

bool SqlQuery::exec(bool throw_ex) {
  const bool ok = QSqlQuery::exec();

  logQuery();

  if (!ok && throw_ex) {
    qCriticalNN << LOGSEC_DB << "SQL exception:" << QUOTE_W_SPACE_DOT(lastError().text());
    THROW_EX(SqlException, lastError());
  }

  return ok;
}

void SqlQuery::logQuery() {
  qDebugNN << LOGSEC_DB << "Executed query:\n" << lastQuery();
}
