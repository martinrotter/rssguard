// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLQUERY_H
#define SQLQUERY_H

#include <QSqlQuery>

class RSSGUARD_DLLSPEC SqlQuery : public QSqlQuery {
  public:
    explicit SqlQuery(const QString& query, const QSqlDatabase& db) = delete;
    explicit SqlQuery(const QSqlDatabase& db);

    bool exec(bool throw_ex = true);
    bool exec(const QString& query, bool throw_ex = true);

  private:
    void logQuery();
};

#endif // SQLQUERY_H
