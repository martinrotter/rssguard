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
    THROW_EX(SqlException, lastError());
  }

  return ok;
}

bool SqlQuery::exec(bool throw_ex) {
  const bool ok = QSqlQuery::exec();

  logQuery();

  if (!ok && throw_ex) {
    THROW_EX(SqlException, lastError());
  }

  return ok;
}

void SqlQuery::logQuery() {
  QString str = lastQuery();

#if QT_VERSION_MAJOR == 5
  QMapIterator<QString, QVariant> it(boundValues());

  while (it.hasNext()) {
    it.next();

    if (it.value().type() == QVariant::Type::Char || it.value().type() == QVariant::Type::String) {
      str.replace(it.key(), QSL("'%1'").arg(it.value().toString()));
    }
    else {
      str.replace(it.key(), it.value().toString());
    }
  }
#endif

  qDebugNN << LOGSEC_DB << "Executed query:\n" << str;
}
