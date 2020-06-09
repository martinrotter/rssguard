// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef IMPORTANTNODE_H
#define IMPORTANTNODE_H

#include "services/abstract/rootitem.h"

class ImportantNode : public RootItem {
  Q_OBJECT

  public:
    explicit ImportantNode(RootItem* parent_item = nullptr);
    virtual ~ImportantNode() = default;

    void updateCounts(bool including_total_count);
    int countOfUnreadMessages() const;
    int countOfAllMessages() const;

  private:
    int m_totalCount{};
    int m_unreadCount{};
};

#endif // IMPORTANTNODE_H
