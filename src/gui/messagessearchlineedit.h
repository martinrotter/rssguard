#ifndef MESSAGESEARCHLINEEDIT_H
#define MESSAGESEARCHLINEEDIT_H

#include "gui/baselineedit.h"


class PlainToolButton;

class MessagesSearchLineEdit : public BaseLineEdit {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesSearchLineEdit(QWidget *parent = 0);
    virtual ~MessagesSearchLineEdit();
};

#endif // MESSAGESEARCHLINEEDIT_H
