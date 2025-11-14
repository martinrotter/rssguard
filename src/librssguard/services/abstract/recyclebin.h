// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RECYCLEBIN_H
#define RECYCLEBIN_H

#include "services/abstract/rootitem.h"

class RSSGUARD_DLLSPEC RecycleBin : public RootItem {
    Q_OBJECT

  public:
    explicit RecycleBin(RootItem* parent_item = nullptr);
    virtual ~RecycleBin() = default;

    virtual QString additionalTooltip() const;
    // virtual QList<QAction*> contextMenuFeedsList();
    virtual void markAsReadUnread(ReadStatus status);
    virtual void cleanMessages(bool clear_only_read);
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;
    virtual void updateCounts();

  public slots:
    virtual void empty();
    virtual void restore();

  private:
    int m_totalCount;
    int m_unreadCount;
    QList<QAction*> m_contextMenu;
};

#endif // RECYCLEBIN_H
