// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef QUITERSSIMPORT_H
#define QUITERSSIMPORT_H

#include <librssguard/core/message.h>

#include <QMutex>
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
    void importArticles(StandardFeed* feed, const QMap<QString, Label*>& lbls);
    void importLabels(const QList<Label*>& labels);
    Message convertArticle(const SqlQuery& rec) const;
    QMap<QString, Label*> hashLabels(const QList<Label*>& labels) const;
    QList<StandardFeed*> importTree(const QSqlDatabase& db, RootItem* root) const;
    RootItem* extractFeedsAndCategories(const QSqlDatabase& db) const;
    QList<Label*> extractLabels(const QSqlDatabase& db) const;
    QIcon decodeBase64Icon(const QString& base64) const;

    void checkIfQuiteRss(const QSqlDatabase& db) const;
    QSqlDatabase dbConnection(const QString& db_file, const QString& connection_name) const;
    void closeDbConnection(QSqlDatabase& db) const;

  private:
    StandardServiceRoot* m_account;
    QString m_dbFile;
    QMutex m_dbMutex;
};

#endif // QUITERSSIMPORT_H
