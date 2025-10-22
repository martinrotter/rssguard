// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef UNREADNODE_H
#define UNREADNODE_H

#include "services/abstract/rootitem.h"

class RSSGUARD_DLLSPEC UnreadNode : public RootItem {
    Q_OBJECT

  public:
    explicit UnreadNode(RootItem* parent_item = nullptr);

    virtual bool cleanMessages(bool clean_read_only);
    virtual void updateCounts(bool including_total_count);
    virtual void markAsReadUnread(ReadStatus status);
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

  private:
    int m_totalCount{};
    int m_unreadCount{};
};

#endif // UNREADNODE_H
