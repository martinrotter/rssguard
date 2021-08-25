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

#endif // TYPEDEFS_H
