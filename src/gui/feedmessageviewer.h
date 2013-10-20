#ifndef FEEDMESSAGEVIEWER_H
#define FEEDMESSAGEVIEWER_H

#include "gui/tabcontent.h"


class WebBrowser;
class FeedsView;
class MessagesView;
class QToolBar;

class FeedMessageViewer : public TabContent {
    Q_OBJECT

  public:
    explicit FeedMessageViewer(QWidget *parent = 0);
    virtual ~FeedMessageViewer();

    WebBrowser *webBrowser();

  protected:
    void initializeViews();

  private:
    QToolBar *m_toolBar;
    MessagesView *m_messagesView;
    FeedsView *m_feedsView;
    WebBrowser *m_messagesBrowser;
};

#endif // FEEDMESSAGEVIEWER_H
