// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLQUERY_H
#define SQLQUERY_H

#include <QSqlQuery>

class RSSGUARD_DLLSPEC LoggedQuery : public QSqlQuery {
  public:
    explicit LoggedQuery(const QString& query = QString(), const QSqlDatabase& db = QSqlDatabase());
    explicit LoggedQuery(const QSqlDatabase& db);

    bool exec(bool throw_ex = true);
    bool exec(const QString& query, bool throw_ex = true);
};

#endif // SQLQUERY_H
