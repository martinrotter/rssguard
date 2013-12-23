#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebPage>


class WebPage : public QWebPage {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit WebPage(QObject *parent = 0);
    virtual ~WebPage();

  protected:
    QWebPage *createWindow(WebWindowType type);
};

#endif // BASEWEBPAGE_H
