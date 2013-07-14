#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QWidget>


class QToolBar;
class QVBoxLayout;
class BaseWebView;
class BaseNetworkAccessManager;

class WebBrowser : public QWidget {
    Q_OBJECT
    
  public:
    explicit WebBrowser(QWidget *parent = 0);
    ~WebBrowser();

    void setupIcons();

    static BaseNetworkAccessManager *globalNetworkManager();
    static QList<WebBrowser*> runningWebBrowsers();

  private:
    QToolBar *m_toolBar;
    QVBoxLayout *m_layout;
    BaseWebView *m_webView;

    QAction *m_actionBack;
    QAction *m_actionForward;
    QAction *m_actionReload;
    QAction *m_actionStop;

    static QPointer<BaseNetworkAccessManager> m_networkManager;
    static QList<WebBrowser*> m_runningWebBrowsers;
};

#endif // WEBBROWSER_H
