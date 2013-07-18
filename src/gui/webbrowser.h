#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QWidget>


class QToolBar;
class QVBoxLayout;
class LocationLineEdit;
class BaseWebView;
class WebBrowserNetworkAccessManager;

class WebBrowser : public QWidget {
    Q_OBJECT
    
  public:
    explicit WebBrowser(QWidget *parent = 0);
    ~WebBrowser();

    void setupIcons();

    static WebBrowserNetworkAccessManager *globalNetworkManager();
    static QList<WebBrowser*> runningWebBrowsers();

  protected:
    void createConnections();

  protected slots:
    void updateUrl(const QUrl &url);
    void navigateToUrl(const QString &url);

  private:
    QVBoxLayout *m_layout;
    QToolBar *m_toolBar;
    BaseWebView *m_webView;
    LocationLineEdit *m_txtLocation;

    QAction *m_actionBack;
    QAction *m_actionForward;
    QAction *m_actionReload;
    QAction *m_actionStop;

    static QPointer<WebBrowserNetworkAccessManager> m_networkManager;
    static QList<WebBrowser*> m_runningWebBrowsers;
};

#endif // WEBBROWSER_H
