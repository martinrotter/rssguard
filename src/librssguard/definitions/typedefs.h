// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QList>
#include <QPair>

#include "core/message.h"
#include "services/abstract/rootitem.h"

// First item here represents ID (int, primary key) of the item.
typedef QList<QPair<int, RootItem*>> Assignment;
typedef QPair<int, RootItem*> AssignmentItem;
typedef QPair<Message, RootItem::Importance> ImportanceChange;

struct ArticleCounts {
    int m_total = -1;
    int m_unread = -1;
};

struct UpdatedArticles {
    QList<Message> m_unread;
    QList<Message> m_all;
};

#endif // TYPEDEFS_H
