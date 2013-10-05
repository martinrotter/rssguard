#ifndef FEEDMESSAGEVIEWER_H
#define FEEDMESSAGEVIEWER_H

#include "gui/tabcontent.h"


class WebBrowser;

class FeedMessageViewer : public TabContent {
    Q_OBJECT

  public:
    explicit FeedMessageViewer(QWidget *parent = 0);
    virtual ~FeedMessageViewer();

     WebBrowser *webBrowser();

  signals:

  public slots:

};

#endif // FEEDMESSAGEVIEWER_H
