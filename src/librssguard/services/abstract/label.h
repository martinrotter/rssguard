// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABEL_H
#define LABEL_H

#include "services/abstract/rootitem.h"

#include <QColor>

class RSSGUARD_DLLSPEC Label : public RootItem {
  Q_OBJECT

  // Added for message filtering with labels.
  Q_PROPERTY(QColor color READ color)

  public:
    explicit Label(const QString& name, const QColor& color, RootItem* parent_item = nullptr);
    explicit Label(RootItem* parent_item = nullptr);

    QColor color() const;
    void setColor(const QColor& color);

    void setCountOfAllMessages(int totalCount);
    void setCountOfUnreadMessages(int unreadCount);

    virtual bool cleanMessages(bool clear_only_read);
    virtual bool markAsReadUnread(ReadStatus status);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool canBeDeleted() const;
    virtual bool deleteViaGui();
    virtual void updateCounts(bool including_total_count);
    virtual QList<Message> undeletedMessages() const;
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
