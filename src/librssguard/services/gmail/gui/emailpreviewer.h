#ifndef EMAILPREVIEWER_H
#define EMAILPREVIEWER_H

#include "services/abstract/gui/custommessagepreviewer.h"

#include "ui_emailpreviewer.h"

class EmailPreviewer : public CustomMessagePreviewer {
  Q_OBJECT

  public:
    explicit EmailPreviewer(QWidget* parent = nullptr);
    virtual ~EmailPreviewer();

    virtual void clear();
    virtual void loadMessage(const Message& msg, RootItem* selected_item);

  private:
    Ui::EmailPreviewer m_ui;
};

#endif // EMAILPREVIEWER_H
