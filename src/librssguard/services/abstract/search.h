// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SEARCH_H
#define SEARCH_H

#include "services/abstract/rootitem.h"

#include <QColor>

class RSSGUARD_DLLSPEC Search : public RootItem {
    Q_OBJECT

    // Added for message filtering with labels.
    Q_PROPERTY(QColor color READ color)

  public:
    enum class Type {
      Regex = 0,
      SqlWhereClause = 1
    };

    explicit Search(const QString& name,
                    Type type,
                    const QString& filter,
                    const QColor& color,
                    RootItem* parent_item = nullptr);
    explicit Search(RootItem* parent_item = nullptr);

    QColor color() const;
    void setColor(const QColor& color);

    QString filter() const;
    void setFilter(const QString& new_filter);

    void setCountOfAllMessages(int totalCount);
    void setCountOfUnreadMessages(int unreadCount);

    virtual void cleanMessages(bool clear_only_read);
    virtual QString additionalTooltip() const;
    virtual void markAsReadUnread(ReadStatus status);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual bool canBeEdited() const;
    virtual bool canBeDeleted() const;
    virtual void deleteItem();
    virtual void updateCounts();

    Type type() const;
    void setType(Type kind);

  private:
    Type m_type;
    QString m_filter;
    QColor m_color;
    int m_totalCount = -1;
    int m_unreadCount = -1;
};

#endif // SEARCH_H
