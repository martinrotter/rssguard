#ifndef BASEWEBPAGE_H
#define BASEWEBPAGE_H

#include <QWebPage>


class BaseWebPage : public QWebPage {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseWebPage(QObject *parent = 0);
    virtual ~BaseWebPage();

  protected:
    QWebPage *createWindow(WebWindowType type);

  private:
    friend class BaseWebView;

    bool m_openInNewTab;
};

#endif // BASEWEBPAGE_H
