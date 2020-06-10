// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef IMPORTANTNODE_H
#define IMPORTANTNODE_H

#include "services/abstract/rootitem.h"

class ImportantNode : public RootItem {
  Q_OBJECT

  public:
    explicit ImportantNode(RootItem* parent_item = nullptr);
    virtual ~ImportantNode() = default;

    QList<Message> undeletedMessages() const;
    bool cleanMessages(bool clean_read_only);
    void updateCounts(bool including_total_count);
    bool markAsReadUnread(ReadStatus status);
    int countOfUnreadMessages() const;
    int countOfAllMessages() const;

  private:
    int m_totalCount{};
    int m_unreadCount{};
};

#endif // IMPORTANTNODE_H
