// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABEL_H
#define LABEL_H

#include "services/abstract/rootitem.h"

#include <QColor>

class RSSGUARD_DLLSPEC Label : public RootItem {
    Q_OBJECT

  public:
    explicit Label(const QString& name, const QIcon& icon, RootItem* parent_item = nullptr);
    explicit Label(RootItem* parent_item = nullptr);

    void setCountOfAllMessages(int totalCount);
    void setCountOfUnreadMessages(int unreadCount);

    virtual void cleanMessages(bool clear_only_read);
    virtual void markAsReadUnread(ReadStatus status);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual bool canBeEdited() const;
    virtual bool canBeDeleted() const;
    virtual void deleteItem();
    virtual void updateCounts();

  public slots:
    void assignToMessage(const Message& msg, bool reload_feeds_model = true);
    void deassignFromMessage(const Message& msg, bool reload_feeds_model = true);

  private:
    int m_totalCount{};
    int m_unreadCount{};
};

#endif // LABEL_H
