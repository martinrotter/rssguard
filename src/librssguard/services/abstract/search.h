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
    explicit Search(const QString& name, const QString& filter, const QColor& color, RootItem* parent_item = nullptr);
    explicit Search(RootItem* parent_item = nullptr);

    QColor color() const;
    void setColor(const QColor& color);

    QString filter() const;
    void setFilter(const QString& new_filter);

    void setCountOfAllMessages(int totalCount);
    void setCountOfUnreadMessages(int unreadCount);

    virtual bool cleanMessages(bool clear_only_read);
    virtual QString additionalTooltip() const;
    virtual bool markAsReadUnread(ReadStatus status);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool canBeDeleted() const;
    virtual bool deleteViaGui();
    virtual void updateCounts(bool including_total_count);
    virtual QList<Message> undeletedMessages() const;

  private:
    QString m_filter;
    QColor m_color;
    int m_totalCount = -1;
    int m_unreadCount = -1;
};

#endif // SEARCH_H
