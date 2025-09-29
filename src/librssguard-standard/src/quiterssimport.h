// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef QUITERSSIMPORT_H
#define QUITERSSIMPORT_H

#include <librssguard/core/message.h>

#include <QObject>
#include <QSqlDatabase>

class RootItem;
class StandardFeed;
class StandardServiceRoot;

class QuiteRssImport : public QObject {
    Q_OBJECT

  public:
    explicit QuiteRssImport(StandardServiceRoot* account, QObject* parent = nullptr);

    void import();

  private:
    void importArticles(StandardFeed* feed);
    Message convertArticle(const QSqlQuery &rec) const;
    QList<StandardFeed*> importTree(QSqlDatabase& db, RootItem* root) const;
    RootItem* extractFeedsAndCategories(QSqlDatabase& db) const;
    QIcon decodeBase64Icon(const QString& base64) const;

    void checkIfQuiteRss(QSqlDatabase& db) const;
    QSqlDatabase dbConnection(const QString& db_file, const QString& connection_name) const;
    void closeDbConnection(QSqlDatabase& db) const;

  private:
    StandardServiceRoot* m_account;
    QString m_dbFile;
};

#endif // QUITERSSIMPORT_H
