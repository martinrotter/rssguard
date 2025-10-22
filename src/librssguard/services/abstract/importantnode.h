// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef IMPORTANTNODE_H
#define IMPORTANTNODE_H

#include "services/abstract/rootitem.h"

class RSSGUARD_DLLSPEC ImportantNode : public RootItem {
    Q_OBJECT

  public:
    explicit ImportantNode(RootItem* parent_item = nullptr);

    virtual bool cleanMessages(bool clean_read_only);
    virtual void updateCounts(bool including_total_count);
    virtual void markAsReadUnread(ReadStatus status);
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

  private:
    int m_totalCount{};
    int m_unreadCount{};
};

#endif // IMPORTANTNODE_H
