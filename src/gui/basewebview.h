#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebView>


class QPaintEvent;
class BaseWebPage;

class BaseWebView : public QWebView {
    Q_OBJECT
  public:
    explicit BaseWebView(QWidget *parent = 0);
    
  protected:
    void paintEvent(QPaintEvent *event);

  private:
    BaseWebPage *m_page;
};

#endif // BASEWEBVIEW_H
