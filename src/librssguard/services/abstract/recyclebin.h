// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RECYCLEBIN_H
#define RECYCLEBIN_H

#include "services/abstract/rootitem.h"

class RecycleBin : public RootItem {
  Q_OBJECT

  public:
    explicit RecycleBin(RootItem* parent_item = nullptr);
    virtual ~RecycleBin() = default;

    QString additionalTooltip() const;

    QList<QAction*> contextMenuFeedsList();
    QList<Message> undeletedMessages() const;

    bool markAsReadUnread(ReadStatus status);
    bool cleanMessages(bool clear_only_read);

    int countOfUnreadMessages() const;
    int countOfAllMessages() const;

    void updateCounts(bool update_total_count);

  public slots:
    virtual bool empty();
    virtual bool restore();

  private:
    int m_totalCount;
    int m_unreadCount;
    QList<QAction*> m_contextMenu;
};

#endif // RECYCLEBIN_H
