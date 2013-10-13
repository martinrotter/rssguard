#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include <QTreeView>


class MessagesView : public QTreeView {
    Q_OBJECT

  public:
    explicit MessagesView(QWidget *parent = 0);
    virtual ~MessagesView();
    
  signals:
    
  public slots:
    
};

#endif // MESSAGESVIEW_H
