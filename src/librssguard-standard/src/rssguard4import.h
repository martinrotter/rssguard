// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RSSGUARD4IMPORT_H
#define RSSGUARD4IMPORT_H

#include <librssguard/core/message.h>

#include <QObject>
#include <QSqlDatabase>

class RootItem;
class Search;
class MessageFilter;
class StandardFeed;
class StandardServiceRoot;

class RssGuard4Import : public QObject {
    Q_OBJECT

  public:
    explicit RssGuard4Import(StandardServiceRoot* account, QObject* parent = nullptr);

    void import();

  private:
    void importArticles(StandardFeed* feed, const QMap<QString, Label*>& lbls);
    void importLabels(const QList<Label*>& labels);
    void importFilters(const QList<MessageFilter*>& filters);
    void importProbes(const QList<Search*>& probes);
    Message convertArticle(const SqlQuery& rec) const;
    QMap<QString, Label*> hashLabels(const QList<Label*>& labels) const;
    QList<StandardFeed*> importTree(RootItem* root) const;
    RootItem* extractFeedsAndCategories(const QSqlDatabase& db) const;
    QList<Label*> extractLabels(const QSqlDatabase& db) const;
    QList<MessageFilter*> extractFilters(const QSqlDatabase& db) const;
    QList<Search*> extractProbes(const QSqlDatabase& db) const;
    QIcon decodeBase64Icon(const QString& base64) const;

    void checkIfRssGuard4(const QSqlDatabase& db) const;
    QSqlDatabase dbConnection(const QString& db_file, const QString& connection_name) const;
    void closeDbConnection(QSqlDatabase& db) const;

  private:
    StandardServiceRoot* m_account;
    QString m_dbFile;
};

#endif // RSSGUARD4IMPORT_H
