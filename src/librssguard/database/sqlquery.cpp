// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/sqlquery.h"

#include "database/databasefactory.h"
#include "exceptions/sqlexception.h"

SqlQuery::SqlQuery(const QString& query, const QSqlDatabase& db) : QSqlQuery(query, db) {}

SqlQuery::SqlQuery(const QSqlDatabase& db) : QSqlQuery(db) {}

bool SqlQuery::exec(bool throw_ex) {
  const bool ok = QSqlQuery::exec();

  DatabaseFactory::logLastExecutedQuery(*this);

  if (!ok && throw_ex) {
    throw SqlException(lastError());
  }

  return ok;
}

bool SqlQuery::exec(const QString& query, bool throw_ex) {
  const bool ok = QSqlQuery::exec(query);

  DatabaseFactory::logLastExecutedQuery(*this);

  if (!ok && throw_ex) {
    throw SqlException(lastError());
  }

  return ok;
}
