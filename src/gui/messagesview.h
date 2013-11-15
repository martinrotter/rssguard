#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include <QTreeView>


class MessagesProxyModel;

class MessagesView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesView(QWidget *parent = 0);
    virtual ~MessagesView();

  protected:
    void setupAppearance();

  private:
    MessagesProxyModel *m_proxyModel;
    
};

#endif // MESSAGESVIEW_H
