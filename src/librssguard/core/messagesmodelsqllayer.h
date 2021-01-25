// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESMODELSQLLAYER_H
#define MESSAGESMODELSQLLAYER_H

#include <QSqlDatabase>

#include <QList>
#include <QMap>

class MessagesModelSqlLayer {
  public:
    explicit MessagesModelSqlLayer();

    // Adds this new state to queue of sort states.
    void addSortState(int column, Qt::SortOrder order);

    // Sets SQL WHERE clause, without "WHERE" keyword.
    void setFilter(const QString& filter);

  protected:
    QString orderByClause() const;
    QString selectStatement() const;
    QString formatFields() const;

    bool isColumnNumeric(int column_id) const;

    QSqlDatabase m_db;

  private:
    QString m_filter;

    // NOTE: These two lists contain data for multicolumn sorting.
    // They are always same length. Most important sort column/order
    // are located at the start of lists;
    QMap<int, QString> m_fieldNames;
    QMap<int, QString> m_orderByNames;
    QList<int> m_sortColumns;
    QList<int> m_numericColumns;
    QList<Qt::SortOrder> m_sortOrders;
};

#endif // MESSAGESMODELSQLLAYER_H
