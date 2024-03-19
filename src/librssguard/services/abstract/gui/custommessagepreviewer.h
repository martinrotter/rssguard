// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CUSTOMMESSAGEPREVIEWER_H
#define CUSTOMMESSAGEPREVIEWER_H

#include "core/message.h"

#include <QWidget>

class RootItem;

class RSSGUARD_DLLSPEC CustomMessagePreviewer : public QWidget {
    Q_OBJECT

  public:
    explicit CustomMessagePreviewer(QWidget* parent = nullptr);
    virtual ~CustomMessagePreviewer();

  public:
    // Clears displayed message.
    virtual void clear() = 0;

    // Displays the message.
    virtual void loadMessage(const Message& msg, RootItem* selected_item) = 0;
};

#endif // CUSTOMMESSAGEPREVIEWER_H
