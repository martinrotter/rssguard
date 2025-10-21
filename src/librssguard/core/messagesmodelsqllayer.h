// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESMODELSQLLAYER_H
#define MESSAGESMODELSQLLAYER_H

#include "core/message.h"

#include <QList>
#include <QMap>
#include <QPair>
#include <QSqlDatabase>

struct SortColumnsAndOrders {
    QList<int> m_columns;
    QList<Qt::SortOrder> m_orders;
};

class MessagesModelSqlLayer {
  public:
    explicit MessagesModelSqlLayer();

    // Adds this new state to queue of sort states.
    void addSortState(int column, Qt::SortOrder order, bool ignore_multicolumn_sorting);

    // Sets SQL WHERE clause, without "WHERE" keyword.
    void setFilter(const QString& filter);

    SortColumnsAndOrders sortColumnAndOrders() const;

  protected:
    QList<Message> fetchMessages(const QHash<QString, Label*>& labels,
                                 int limit,
                                 int offset,
                                 int additional_article_id = -1) const;

    QSqlDatabase m_db;

  private:
    QString selectStatement(int limit, int offset, int additional_article_id = -1) const;
    QString orderByClause() const;
    QString limitOffset(int limit, int offset) const;
    QString formatFields() const;
    int mapColumnToDatabase(int column) const;

    QString m_filter;
    QStringList m_fieldNames;
    QStringList m_orderByNames;
    QList<int> m_sortColumns;
    QList<Qt::SortOrder> m_sortOrders;
};

#endif // MESSAGESMODELSQLLAYER_H
