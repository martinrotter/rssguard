// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESEARCHLINEEDIT_H
#define MESSAGESEARCHLINEEDIT_H

#include "gui/baselineedit.h"

class PlainToolButton;

class MessagesSearchLineEdit : public BaseLineEdit {
  Q_OBJECT

  public:
    explicit MessagesSearchLineEdit(QWidget* parent = nullptr);
};

#endif // MESSAGESEARCHLINEEDIT_H
