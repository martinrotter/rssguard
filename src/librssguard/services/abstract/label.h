// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABEL_H
#define LABEL_H

#include "services/abstract/rootitem.h"

#include <QColor>

class Label : public RootItem {
  Q_OBJECT

  public:
    explicit Label(const QString& name, const QColor& color, RootItem* parent_item = nullptr);
    explicit Label(RootItem* parent_item = nullptr);

    QColor color() const;
    void setColor(const QColor& color);

    void setCountOfAllMessages(int totalCount);
    void setCountOfUnreadMessages(int unreadCount);

    virtual bool markAsReadUnread(ReadStatus status);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool canBeDeleted() const;
    virtual bool deleteViaGui();
    virtual void updateCounts(bool including_total_count);
    static QIcon generateIcon(const QColor& color);

  public slots:
    void assignToMessage(const Message& msg);
    void deassignFromMessage(const Message& msg);

  private:
    QColor m_color;
    int m_totalCount{};
    int m_unreadCount{};
};

#endif // LABEL_H
